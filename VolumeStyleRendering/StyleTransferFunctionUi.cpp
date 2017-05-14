#include "StyleTransferFunctionUi.h"
#include "RaycastVolume.h"
#include "CinderImGui.h"
#include "cinder/ip/Resize.h"
using namespace glm;
using namespace ci;

void StyleTransferFunctionUi::drawThresholdControl() const
{
    // draw threshold indicator
    const ImVec2 p = ui::GetCursorScreenPos();
    ImDrawList* drawList = ui::GetWindowDrawList();
    auto width = ui::GetContentRegionAvailWidth();
    ImU32 gray = ImColor(0.5f, 0.5f, 0.5f, 0.5f);
    float x = p.x;
    float y = p.y - 35.0f - 4.0f;
    float steps = width / 255;
    ImGuiStyle& style = ui::GetStyle();
    auto threshold = transferFunction->getThreshold();

    if (threshold.x > 0)
    {
        drawList->AddRectFilled(ImVec2(x, y - 120), ImVec2(x + threshold.x * steps, y), gray, style.FrameRounding);
    }

    if (threshold.y < 255)
    {
        drawList->AddRectFilled(ImVec2(x + threshold.y * steps, y - 120), ImVec2(x + 255 * steps, y), gray,
                                style.FrameRounding);
    }

    // draw threshold controls
    ui::BeginGroup();
    ui::PushItemWidth(-2.0f);

    if (ui::SliderInt2("##Threshold", value_ptr(threshold), 0, 255))
    {
        transferFunction->setThreshold(threshold.x, threshold.y);
        transferFunction->updateFunction();
    }

    ui::PopItemWidth();
    ui::EndGroup();
}

void StyleTransferFunctionUi::drawUi(bool& open, const RaycastVolume& volume)
{
    if (open)
    {
        ui::Begin("Transfer Function", &open, ImGuiWindowFlags_AlwaysAutoResize);
        drawHistogram(volume);
        drawControlPointsUi();
        drawThresholdControl();
        drawControlPointCreationUi();
        ui::End();
    }
}

StyleTransferFunctionUi::StyleTransferFunctionUi()
{
    transferFunction = std::make_shared<StyleTransferFunction>();
}

StyleTransferFunctionUi::~StyleTransferFunctionUi() {}

const std::shared_ptr<StyleTransferFunction> &StyleTransferFunctionUi::getTranferFunction() const
{
    return transferFunction;
}

void StyleTransferFunctionUi::drawHistogram(const RaycastVolume& volume) const
{
    ui::BeginGroup();
    auto& histogram = volume.getHistogram();
    ui::PlotHistogram("##histogram", histogram.data(), histogram.size(), 0, nullptr, 0.0f, 1.0f, ImVec2(520, 120));
    ui::EndGroup();
}

int StyleTransferFunctionUi::stylesManagerPopup() const
{
    auto selectedStyle = -2;
    auto styleCount = "   (" + std::to_string(Style::GetAvailableStyles().size()) + "/128)";

    if (ui::BeginPopupModal("Styles Manager", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
    {
        ui::BeginChild("##styleList", ImVec2(app::getWindowWidth() * .3, app::getWindowHeight() * .7), true);
        static ImU32 gray = ImColor(0.5f, 0.5f, 0.5f, 0.5f);
        ImDrawList* drawList = ui::GetWindowDrawList();
        ImGuiStyle& uiStyle = ui::GetStyle();

        // copy vector of styles
        auto styles = Style::GetAvailableStyles();

        for (int i = 0; i < styles.size(); i++)
        {
            auto& style = styles[i];
            auto cursor = ui::GetCursorScreenPos();
            auto name = style.getName();

            // background rect for style
            drawList->AddRectFilled(cursor, ImVec2(cursor.x + ui::GetContentRegionAvailWidth(), cursor.y + 76),
                                    gray, uiStyle.FrameRounding);

            ui::Dummy(ImVec2(0, 4));
            ui::BeginGroup();
            ui::Indent(4);
            ui::Image((void *)(intptr_t)style.getTexture()->getId(), ImVec2(64, 64));

            if (ui::IsItemHovered())
            {
                ui::SetTooltip(style.getFilepath().c_str());
            }

            ui::Unindent(4);
            ui::EndGroup();
            ui::SameLine();
            ui::PushID(style.getFilepath().c_str());
            ui::BeginGroup();

            if (ui::InputText("Name", &name)) { Style::RenameStyle(i, name); }

            int inputHeight = ui::GetItemRectSize().y;
            int width = ui::GetContentRegionAvailWidth();

            if (ui::Button("Select", ImVec2(width * .5 - 6, 64 - inputHeight)))
            {
                ui::CloseCurrentPopup();
                selectedStyle = i;
            }

            ui::SameLine();

            if (i > 0 && ui::Button("Delete", ImVec2(width * .5 - 6, 64 - inputHeight)))
            {
                Style::RemoveStyle(i--);
            }

            ui::EndGroup();
            ui::PopID();
            ui::Dummy(ImVec2(ui::GetContentRegionAvailWidth(), 4));
        }

        ui::EndChild();
        ui::TextDisabled(styleCount.c_str());

        if (ui::Button("Load Litsphere / Matcap", ImVec2(ui::GetContentRegionAvailWidth(), 0)))
        {
            static fs::path fsPath;
            fsPath = app::getOpenFilePath(fsPath, {"png", "jpg", "bmp"});

            if (!fsPath.empty())
            {
                Surface baseImage = loadImage(fsPath);
                Surface resizedImage(512, 512, true, SurfaceChannelOrder::RGBA);
                ip::resize(baseImage, &resizedImage);

                Style::AddStyle(Style(fsPath.filename().string(), resizedImage,
                                      gl::Texture2d::create(resizedImage), fsPath.string()));
            }
        }

        if (ui::Button("Cancel", ImVec2(ui::GetContentRegionAvailWidth(), 0)))
        {
            ui::CloseCurrentPopup();
            selectedStyle = -1;
        }

        ui::EndPopup();
    }

    return selectedStyle;
}

void StyleTransferFunctionUi::drawControlPointsUi() const
{
    const ImVec2 p = ui::GetCursorScreenPos();
    ImDrawList* drawList = ui::GetWindowDrawList();
    int width = ui::GetContentRegionAvailWidth() - 4.0f;
    ImU32 white = ImColor(1.0f, 1.0f, 1.0f, 1.0f);
    ImU32 black = ImColor(0.0f, 0.0f, 0.0f, 1.0f);
    float sz = 20.0f;
    float x = p.x + 4.0f;
    float y = p.y + 4.0f;
    float step = static_cast<float>(width) / 256;
    auto& colorPoints = transferFunction->getColorPoints();
    auto& alphaPoints = transferFunction->getAlphaPoints();
    auto& stylePoints = transferFunction->getStylePoints();

    for (int i = 0; i < 256; i++)
    {
        const auto& color = transferFunction->getColor(i);
        ImU32 col32 = ImColor(color);
        // draw color transfer function
        drawList->AddLine(ImVec2(x, y), ImVec2(x, y + sz), col32, 2.0f * step);
        x += step;
    }

    for (auto& colorP : colorPoints)
    {
        const auto& color = colorP.getColor();
        ImU32 col32 = ImColor(color.r, color.g, color.b, 1.0f);
        ImU32 invCol32 = ImColor(1.0f - color.r, 1.0f - color.g, 1.0f - color.b, 1.0f);
        drawList->AddCircleFilled(ImVec2(p.x + step * colorP.getIsoValue() + 4.0f, y + sz), 5, col32, 24);
        drawList->AddCircle(ImVec2(p.x + step * colorP.getIsoValue() + 4.0f, y + sz), 5, invCol32, 24);
    }

    for (auto& styleP : stylePoints)
    {
        const auto img = (void*)(intptr_t)styleP.getStyle().getTexture()->getId();
        drawList->AddImage(img, ImVec2(p.x + step * styleP.getIsoValue() + 4.0f - 8, y - 8),
                           ImVec2(p.x + step * styleP.getIsoValue() + 4.0f + 16 - 8, y + 16 - 8));
    }

    for (int i = 0; i < 255; i++)
    {
        auto aBegin = clamp(1.0f - transferFunction->getColor(i).a, 0.0f, 1.0f);
        auto aEnd = clamp(1.0f - transferFunction->getColor(i + 1).a, 0.0f, 1.0f);
        drawList->AddLine(ImVec2(p.x + step * i + 4.0f, y - 124.0f + aBegin * 120),
                          ImVec2(p.x + step * (i + 1) + 4.0f, y - 124.0f + aEnd * 120), white, 2);
    }

    for (auto& alphaP : alphaPoints)
    {
        const auto& alpha = 1.0f - alphaP.getAlpha();
        ImU32 opacity = ImColor(1.0f - vec4(alpha));
        drawList->AddCircleFilled(ImVec2(p.x + step * alphaP.getIsoValue() + 4.0f, y - 124.0f + alpha * 120),
                                  5, opacity, 24);
        drawList->AddCircle(ImVec2(p.x + step * alphaP.getIsoValue() + 4.0f, y - 124.0f + alpha * 120),
                            5, black, 24);
    }

    ui::Dummy(ImVec2(width, sz + 15.0f));
}

void StyleTransferFunctionUi::drawPointIsoValueControl(const TransferFunctionPoint& p, const int index, int pointType) const
{
    int pIso = p.getIsoValue();
    ui::SameLine();
    ui::PushItemWidth(-30);

    if (p.getIsoValue() != 0 && p.getIsoValue() != 255)
    {
        if (ui::SliderInt("##isoValA", &pIso, 1, 254))
        {
            pIso = min(max(pIso, 1), 254);
            pointType == 0 ? transferFunction->setAlphaPointIsoValue(index, pIso)
                : pointType == 1 ? transferFunction->setColorPointIsoValue(index, pIso)
                : transferFunction->setStylePointIsoValue(index, pIso);
        }
    }
    else if (p.getIsoValue() == 0 || p.getIsoValue() == 255)
    {
        ui::PushStyleColor(ImGuiCol_Button, ImVec4(vec4(0.5f)));
        ui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(vec4(0.5f)));
        ui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(vec4(0.5f)));
        ui::Button(std::to_string(p.getIsoValue()).c_str(),
                   ImVec2(ui::GetContentRegionAvailWidth() - 30, 0));
        ui::PopStyleColor(3);
    }

    ui::PopItemWidth();
}

void StyleTransferFunctionUi::drawAlphaPointList() const
{
    for (auto i = 0; i < transferFunction->getAlphaPoints().size(); i++)
    {
        auto& alphaP = transferFunction->getAlphaPoints()[i];
        auto pAlpha = alphaP.getAlpha();

        ui::PushID(&alphaP);
        ui::PushItemWidth(ui::GetContentRegionAvailWidth() * 0.5f);

        if (ui::DragFloat("##alpha", &pAlpha, 0.01f, 0.0f, 1.0f))
        {
            transferFunction->setAlpha(i, pAlpha);
        }

        ui::PopItemWidth();
        drawPointIsoValueControl(alphaP, i, 0);

        if (alphaP.getIsoValue() != 0 && alphaP.getIsoValue() != 255)
        {
            ui::SameLine();

            if (ui::Button("X", ImVec2(ui::GetContentRegionAvailWidth(), 0)))
            {
                transferFunction->removeAlphaPoint(i--);
            }
        }

        ui::PopID();
    }
}

void StyleTransferFunctionUi::drawColorPointList() const
{
    for (auto i = 0; i < transferFunction->getColorPoints().size(); i++)
    {
        auto& colorP = transferFunction->getColorPoints()[i];
        auto pColor = colorP.getColor();

        ui::PushID(&colorP);
        ui::PushItemWidth(ui::GetContentRegionAvailWidth() * 0.5f);

        if (ui::ColorEdit3("##color", value_ptr(pColor)))
        {
            transferFunction->setColor(i, pColor);
        }

        ui::PopItemWidth();
        drawPointIsoValueControl(colorP, i, 1);

        if (colorP.getIsoValue() != 0 && colorP.getIsoValue() != 255)
        {
            ui::SameLine();

            if (ui::Button("X", ImVec2(ui::GetContentRegionAvailWidth(), 0)))
            {
                transferFunction->removeColorPoint(i--);
            }
        }

        ui::PopID();
    }
}

void StyleTransferFunctionUi::drawStylePointList() const
{
    static bool showStyleManager = false;
    static int stylePointChangeIndex = -1;
    static int height = 1;

    for (auto i = 0; i < transferFunction->getStylePoints().size(); i++)
    {
        auto& styleP = transferFunction->getStylePoints()[i];
        // copy style struct
        auto style = styleP.getStyle();

        ui::PushID(&styleP);
        ui::Image((void *)(intptr_t)style.getTexture()->getId(), ImVec2(height, height));
        ui::SameLine();
        ui::PushItemWidth(ui::GetContentRegionAvailWidth() * 0.4f - 32);

        auto name = style.getName();

        if (ui::InputText("##name", &name)) { Style::RenameStyle(styleP.getStyleIndex(), name); }

        ui::PopItemWidth();
        ui::SameLine();

        if (ui::Button("Change Style"))
        {
            showStyleManager = true;
            stylePointChangeIndex = i;
        }

        height = ui::GetItemRectSize().y;
        drawPointIsoValueControl(styleP, i, 2);
        ui::SameLine();

        if (ui::Button("X", ImVec2(ui::GetContentRegionAvailWidth(), 0)))
        {
            transferFunction->removeStylePoint(i--);
        }

        ui::PopID();
    }

    if (showStyleManager && stylePointChangeIndex >= 0)
    {
        ui::OpenPopup("Styles Manager");
        auto styleIndex = stylesManagerPopup();
        showStyleManager = styleIndex == -2;

        if (styleIndex >= 0)
        {
            transferFunction->setStylePoint(stylePointChangeIndex, styleIndex);
            stylePointChangeIndex = -1;
        }
    }
}

void StyleTransferFunctionUi::drawControlPointList(int pointType) const
{
    // draw control point list
    if (pointType == 0 ? ui::TreeNode("Alpha Points")
            : pointType == 1 ? ui::TreeNode("Color Points")
            : ui::TreeNode("Style Points"))
    {
        ui::BeginGroup();
        ui::BeginChild(ui::GetID((void*)(intptr_t)pointType), ImVec2(ui::GetContentRegionAvailWidth(), 120), true);

        if (pointType == 0)
        {
            drawAlphaPointList();
        }
        else if (pointType == 1)
        {
            drawColorPointList();
        }
        else if (pointType == 2)
        {
            drawStylePointList();
        }

        ui::EndChild();
        ui::EndGroup();
        ui::TreePop();
    }
}

void StyleTransferFunctionUi::drawControlPointCreationUi() const
{
    // creation bar
    static int pointType = 0;
    static int isoValue = 127;
    static float alpha;
    static vec3 color;
    static bool showStylesManager = false;
    static StylePoint stylePoint;

    ui::BeginGroup();
    ui::Separator();
    ui::Text("Point Type: ");
    ui::SameLine();
    ui::RadioButton("Alpha", &pointType, 0);
    ui::SameLine();
    ui::RadioButton("Color", &pointType, 1);
    ui::SameLine();
    ui::RadioButton("Style", &pointType, 2);

    // iso value bar
    ui::SliderInt("Iso Value", &isoValue, 1, 254);
    ImVec2 sliderSize = ui::GetItemRectSize();

    // alpha point
    if (pointType == 0)
    {
        ui::DragFloat("Alpha", &alpha, 0.01f, 0.0f, 1.0f);
    }
    else if (pointType == 1) // color point
    {
        ui::ColorEdit3("Color", value_ptr(color));
    }
    else if (pointType == 2) // style point
    {
        if (stylePoint.getStyleIndex() >= 0)
        {
            stylePoint.setIsoValue(isoValue);
            auto& style = stylePoint.getStyle();
            ui::Image((void *)(intptr_t)style.getTexture()->getId(), ImVec2(sliderSize.y, sliderSize.y));
            ui::SameLine();

            if (ui::Button(style.getName().c_str(), ImVec2(sliderSize.x - sliderSize.y - 2, sliderSize.y)))
            {
                showStylesManager = true;
            }
        }
        else
        {
            if (ui::Button("Select Style", sliderSize)) { showStylesManager = true; }
        }
    }

    ui::EndGroup();
    ImVec2 size = ui::GetItemRectSize();
    ui::SameLine();
    ui::BeginGroup();

    if (pointType == 2 && stylePoint.getStyleIndex() < 0)
    {
        ui::PushStyleColor(ImGuiCol_Button, ImVec4(vec4(0.5f)));
        ui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(vec4(0.5f)));
        ui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(vec4(0.5f)));
    }

    if (ui::Button("Add Point", ImVec2(ui::GetContentRegionAvailWidth(), size.y / 2)))
    {
        pointType == 0 ? transferFunction->addAlphaPoint(alpha, isoValue)
            : pointType == 1 ? transferFunction->addColorPoint(color, isoValue) : 0;

        if (pointType == 2 && stylePoint.getStyleIndex() >= 0) { transferFunction->addStylePoint(stylePoint); }
    }

    if (pointType == 2 && stylePoint.getStyleIndex() < 0) { ui::PopStyleColor(3); }

    if (ui::Button("Styles Manager", ImVec2(ui::GetContentRegionAvailWidth(), size.y / 2)))
    {
        showStylesManager = true;
    }

    if (showStylesManager)
    {
        ui::OpenPopup("Styles Manager");
        auto selectedStyleIndex = stylesManagerPopup();
        showStylesManager = selectedStyleIndex == -2;

        if (selectedStyleIndex >= 0) stylePoint = StylePoint(isoValue, selectedStyleIndex);
    }

    ui::EndGroup();
    drawControlPointList(pointType);
}
