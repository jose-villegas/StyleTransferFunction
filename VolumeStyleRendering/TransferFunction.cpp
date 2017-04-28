#include "TransferFunction.h"

#include "CinderImGui.h"
#include "Volume3D.h"
using namespace glm;

void TransferFunction::insertLimitPoints(const int limit)
{
    colorPoints[0] = TransferFunctionColorPoint(vec3(1), 0);
    colorPoints[limit] = TransferFunctionColorPoint(vec3(1), limit);

    alphaPoints[0] = TransferFunctionAlphaPoint(1.0, 0);
    alphaPoints[limit] = TransferFunctionAlphaPoint(1.0, limit);
}

TransferFunction::TransferFunction(const int limit) : volume(nullptr)
{
    insertLimitPoints(limit);
}

TransferFunction::TransferFunction() : volume(nullptr)
{
    insertLimitPoints(std::numeric_limits<uint8_t>().max());
}

TransferFunction::~TransferFunction() {}

void TransferFunction::drawControlPointList(int& pointType)
{
    // draw control point list
    if (pointType == 1 && ui::TreeNode("Color Points"))
    {
        ui::BeginGroup();
        ui::BeginChild(ui::GetID((void*)(intptr_t)pointType), ImVec2(ui::GetContentRegionAvailWidth(), 120), true);

        for (auto it = colorPoints.begin(); it != colorPoints.end();)
        {
            ui::PushID(it->first);
            auto pColor = it->second.getColor();
            auto pIso = it->second.getIsoValue();
            bool deletedValue = false;

            if (ui::ColorEdit3("##color", value_ptr(pColor)))
            {
                it->second.setColor(pColor);
            }

            ui::SameLine();
            ui::PushItemWidth(-30);

            if (it->first != 0 && it->first != 255)
            {
                if (ui::InputInt("##isoValA", &pIso, 1, 5, ImGuiInputTextFlags_EnterReturnsTrue))
                {
                    pIso = min(max(pIso, 1), 254);
                    bool alreadyExist = colorPoints.find(pIso) != colorPoints.end();

                    if (!alreadyExist)
                    {
                        it->second.setIsoValue(pIso);
                        std::swap(colorPoints[pIso], it->second);
                        colorPoints.erase(it++);
                        deletedValue = true;
                    }
                }
            }
            else if (it->first == 0 || it->first == 255)
            {
                ui::PushStyleColor(ImGuiCol_Button, ImVec4(vec4(0.5f)));
                ui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(vec4(0.5f)));
                ui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(vec4(0.5f)));
                ui::Button(std::to_string(it->first).c_str(), ImVec2(ui::GetContentRegionAvailWidth() - 30, 0));
                ui::PopStyleColor(3);
            }

            ui::PopItemWidth();

            if (it->first != 0 && it->first != 255)
            {
                ui::SameLine();

                if (ui::Button("X", ImVec2(ui::GetContentRegionAvailWidth(), 0)))
                {
                    colorPoints.erase(it++);
                    deletedValue = true;
                }
            }

            if (!deletedValue) ++it;
            ui::PopID();
        }

        ui::EndChild();
        ui::EndGroup();
        ui::TreePop();
    }
    else if (pointType == 0 && ui::TreeNode("Alpha Points"))
    {
        ui::BeginGroup();
        ui::BeginChild(ui::GetID((void*)(intptr_t)pointType), ImVec2(ui::GetContentRegionAvailWidth(), 120), true);

        for (auto it = alphaPoints.begin(); it != alphaPoints.end();)
        {
            auto pAlpha = it->second.getAlpha();
            auto pIso = it->second.getIsoValue();
            bool deletedValue = false;
            ui::PushID(it->first);

            if (ui::DragFloat("##alpha", &pAlpha, 0.01f, 0.0f, 1.0f))
            {
                it->second.setAlpha(pAlpha);
            }

            ui::SameLine();
            ui::PushItemWidth(-30);

            if (it->first != 0 && it->first != 255)
            {
                if (ui::InputInt("##isoValA", &pIso, 1, 5, ImGuiInputTextFlags_EnterReturnsTrue))
                {
                    pIso = min(max(pIso, 1), 254);
                    bool alreadyExist = alphaPoints.find(pIso) != alphaPoints.end();

                    if (!alreadyExist)
                    {
                        it->second.setIsoValue(pIso);
                        std::swap(alphaPoints[pIso], it->second);
                        alphaPoints.erase(it++);
                        deletedValue = true;
                    }
                }
            }
            else if (it->first == 0 || it->first == 255)
            {
                ui::PushStyleColor(ImGuiCol_Button, ImVec4(vec4(0.5f)));
                ui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(vec4(0.5f)));
                ui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(vec4(0.5f)));
                ui::Button(std::to_string(it->first).c_str(), ImVec2(ui::GetContentRegionAvailWidth() - 30, 0));
                ui::PopStyleColor(3);
            }

            ui::PopItemWidth();

            if (it->first != 0 && it->first != 255)
            {
                ui::SameLine();

                if (ui::Button("X", ImVec2(ui::GetContentRegionAvailWidth(), 0)))
                {
                    alphaPoints.erase(it++);
                    deletedValue = true;
                }
            }

            if (!deletedValue) ++it;
            ui::PopID();
        }

        ui::EndChild();
        ui::EndGroup();
        ui::TreePop();
    }
}

void TransferFunction::drawControlPointCreationUi()
{
    // creation bar
    static int pointType = 0;
    static int isoValue = 0;
    static float alpha;
    static vec3 color;
    ui::BeginGroup();
    ui::Separator();
    ui::RadioButton("Alpha Point", &pointType, 0);
    ui::SameLine();
    ui::RadioButton("Color Point", &pointType, 1);
    // iso value bar
    ui::SliderInt("Iso Value", &isoValue, 0, 255);

    // alpha point
    if (pointType == 0)
    {
        ui::DragFloat("Alpha", &alpha, 0.01f, 0.0f, 1.0f);
    }
    else // color point
    {
        ui::ColorEdit3("Color", value_ptr(color));
    }
    ui::EndGroup();

    ImVec2 size = ImGui::GetItemRectSize();
    size.x = 512 - size.x;
    ui::SameLine();

    if (ui::Button("Add Point", size))
    {
        if (pointType == 0)
        {
            alphaPoints[isoValue] = TransferFunctionAlphaPoint(alpha, isoValue);
        }
        else
        {
            colorPoints[isoValue] = TransferFunctionColorPoint(color, isoValue);
        }
    }

    drawControlPointList(pointType);
}

void TransferFunction::drawColorPointsUi()
{
    const ImVec2 p = ui::GetCursorScreenPos();
    ImDrawList* drawList = ui::GetWindowDrawList();
    ImU32 col32 = ImColor(1.0f, 1.0f, 1.0f, 1.0f);
    float sz = 20.0f;
    float x = p.x + 4.0f;
    float y = p.y + 4.0f;

    for (int i = 0; i < 256; i++)
    {
        drawList->AddLine(ImVec2(x, y), ImVec2(x, y + sz), col32, 3);
        x += 2.0f;
    }

    ui::Dummy(ImVec2(520, sz + 8.0f));
}

void TransferFunction::drawHistogram()
{
    auto& histogram = volume->getHistogram();
    ui::PushItemWidth(-1);
    ui::PlotHistogram("##histogram", histogram.data(), histogram.size(), 0, nullptr, 0.0f, 1.0f, ImVec2(520, 120));
    ui::PopItemWidth();
}

void TransferFunction::drawUi(bool& open)
{
    ui::ShowTestWindow();

    if (!ui::Begin("Transfer Function", &open, ImGuiWindowFlags_AlwaysAutoResize))
    {
        ui::End();
    }

    if (volume != nullptr)
    {
        drawHistogram();
        drawColorPointsUi();
        drawControlPointCreationUi();
    }

    ui::End();
}

void TransferFunction::setLimit(const int limit)
{
    alphaPoints.end()->second.setIsoValue(limit);
    colorPoints.end()->second.setIsoValue(limit);
}

void TransferFunction::addColorPoint(const vec3& color, const int isoValue)
{
    // inserted point is off limits
    if (isoValue <= 0 || isoValue >= colorPoints.end()->second.getIsoValue())
    {
        return;
    }

    colorPoints[isoValue] = TransferFunctionColorPoint(color, isoValue);
}

void TransferFunction::addAlphaPoint(const float alpha, const int isoValue)
{
    // inserted point is off limits
    if (isoValue <= 0 || isoValue >= alphaPoints.end()->second.getIsoValue())
    {
        return;
    }

    alphaPoints[isoValue] = TransferFunctionAlphaPoint(alpha, isoValue);
}

void TransferFunction::removeColorPoint(const int isoValue)
{
    // off limits
    if (isoValue <= 0 || isoValue >= colorPoints.end()->second.getIsoValue())
    {
        return;
    }

    auto it = colorPoints.find(isoValue);

    if (it != colorPoints.end())
    {
        colorPoints.erase(it);
    }
}

void TransferFunction::removeAlphaPoint(const int isoValue)
{
    // off limits
    if (isoValue <= 0 || isoValue >= alphaPoints.end()->second.getIsoValue())
    {
        return;
    }

    auto it = alphaPoints.find(isoValue);

    if (it != alphaPoints.end())
    {
        alphaPoints.erase(it);
    }
}

void TransferFunction::setVolume(const Volume3D& volume)
{
    this->volume = &volume;
}
