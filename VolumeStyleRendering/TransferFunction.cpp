#include "TransferFunction.h"

#include "CinderImGui.h"
#include "Volume3D.h"
using namespace glm;

void TransferFunction::insertLimitPoints(const int limit)
{
    colorPoints.push_back(TransferFunctionColorPoint(vec3(1), 0));
    colorPoints.push_back(TransferFunctionColorPoint(vec3(1), limit));

    alphaPoints.push_back(TransferFunctionAlphaPoint(1.0, 0));
    alphaPoints.push_back(TransferFunctionAlphaPoint(1.0, limit));

    updateSplines();
}


TransferFunction::TransferFunction() : volume(nullptr)
{
    insertLimitPoints(255);
}

TransferFunction::~TransferFunction() {}

void TransferFunction::drawControlPointList(int& pointType)
{
    bool deleteCtrlPoint = false;
    bool sortPoints = false;

    auto isoValueControl = [&](TransferFunctionPoint& p)
            {
                int pIso = p.getIsoValue();
                ui::SameLine();
                ui::PushItemWidth(-30);

                if (p.getIsoValue() != 0 && p.getIsoValue() != 255)
                {
                    if (ui::SliderInt("##isoValA", &pIso, 1, 254))
                    {
                        pIso = min(max(pIso, 1), 254);
                        p.setIsoValue(pIso);
                        sortPoints = true;
                    }
                }
                else if (p.getIsoValue() == 0 || p.getIsoValue() == 255)
                {
                    ui::PushStyleColor(ImGuiCol_Button, ImVec4(vec4(0.5f)));
                    ui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(vec4(0.5f)));
                    ui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(vec4(0.5f)));
                    ui::Button(std::to_string(p.getIsoValue()).c_str(), ImVec2(ui::GetContentRegionAvailWidth() - 30, 0));
                    ui::PopStyleColor(3);
                }

                ui::PopItemWidth();

                if (p.getIsoValue() != 0 && p.getIsoValue() != 255)
                {
                    ui::SameLine();

                    if (ui::Button("X", ImVec2(ui::GetContentRegionAvailWidth(), 0)))
                    {
                        deleteCtrlPoint = true;
                    }
                }
            };

    // draw control point list
    if (pointType == 1 && ui::TreeNode("Color Points"))
    {
        ui::BeginGroup();
        ui::BeginChild(ui::GetID((void*)(intptr_t)pointType), ImVec2(ui::GetContentRegionAvailWidth(), 120), true);

        for (auto it = colorPoints.begin(); it != colorPoints.end();)
        {
            ui::PushID(&(*it));
            auto pColor = it->getColor();

            ui::PushItemWidth(ui::GetContentRegionAvailWidth() * 0.5f);

            if (ui::ColorEdit3("##color", value_ptr(pColor)))
            {
                it->setColor(pColor);
                updateSplines();
            }

            ui::PopItemWidth();

            isoValueControl(*it);
            deleteCtrlPoint ? it = colorPoints.erase(it) : ++it;

            if (deleteCtrlPoint)
            {
                updateSplines();
                deleteCtrlPoint = false;
            }

            ui::PopID();
        }

        ui::EndChild();
        ui::EndGroup();
        ui::TreePop();

        if (sortPoints)
        {
            // sort by iso value
            sort(colorPoints.begin(), colorPoints.end());
            // update
            updateSplines();
        }
    }
    else if (pointType == 0 && ui::TreeNode("Alpha Points"))
    {
        ui::BeginGroup();
        ui::BeginChild(ui::GetID((void*)(intptr_t)pointType), ImVec2(ui::GetContentRegionAvailWidth(), 120), true);
        
        for ( auto it = alphaPoints.begin(); it != alphaPoints.end();)
        {
            ui::PushID(&(*it));
            auto pAlpha = it->getAlpha();

            ui::PushItemWidth(ui::GetContentRegionAvailWidth() * 0.5f);

            if (ui::DragFloat("##alpha", &pAlpha, 0.01f, 0.0f, 1.0f))
            {
                it->setAlpha(pAlpha);
                updateSplines();
            }

            ui::PopItemWidth();

            isoValueControl(*it);
            deleteCtrlPoint ? it = alphaPoints.erase(it) : ++it;

            if (deleteCtrlPoint)
            {
                updateSplines();
                deleteCtrlPoint = false;
            }

            ui::PopID();
        }

        ui::EndChild();
        ui::EndGroup();
        ui::TreePop();

        if (sortPoints)
        {
            // sort by iso value
            sort(alphaPoints.begin(), alphaPoints.end());
            // update
            updateSplines();
        }
    }
}

std::vector<TransferFunction::Cubic> TransferFunction::calculateCubicSpline(std::vector<vec3> points)
{
    auto n = points.size() - 1;
    auto& v = points;
    std::vector<vec3> gamma(n + 1);
    std::vector<vec3> delta(n + 1);
    std::vector<vec3> D(n + 1);

    int i;
    /* We need to solve the equation
    * taken from: http://mathworld.wolfram.com/CubicSpline.html
    [2 1       ] [D[0]]   [3(v[1] - v[0])  ]
    |1 4 1     | |D[1]|   |3(v[2] - v[0])  |
    |  1 4 1   | | .  | = |      .         |
    |    ..... | | .  |   |      .         |
    |     1 4 1| | .  |   |3(v[n] - v[n-2])|
    [       1 2] [D[n]]   [3(v[n] - v[n-1])]

    by converting the matrix to upper triangular.
    The D[i] are the derivatives at the control points.
    */

    //this builds the coefficients of the left matrix
    gamma[0] = vec3(0);
    gamma[0].x = 1.0f / 2.0f;
    gamma[0].y = 1.0f / 2.0f;
    gamma[0].z = 1.0f / 2.0f;

    for (i = 1; i < n; i++)
    {
        gamma[i] = vec3(1) / ((4.0f * vec3(1)) - gamma[i - 1]);
    }

    gamma[n] = vec3(1) / ((2.0f * vec3(1)) - gamma[n - 1]);

    delta[0] = 3.0f * (v[1] - v[0]) * gamma[0];

    for (i = 1; i < n; i++)
    {
        delta[i] = (3.0f * (v[i + 1] - v[i - 1]) - delta[i - 1]) * gamma[i];
    }

    delta[n] = (3.0f * (v[n] - v[n - 1]) - delta[n - 1]) * gamma[n];

    D[n] = delta[n];

    for (i = n - 1; i >= 0; i--)
    {
        D[i] = delta[i] - gamma[i] * D[i + 1];
    }

    // now compute the coefficients of the cubics 
    std::vector<Cubic> C(n);

    for (i = 0; i < n; i++)
    {
        C[i] = Cubic(v[i], D[i], 3.0f * (v[i + 1] - v[i]) - 2.0f * D[i] - D[i + 1], 2.0f * (v[i] - v[i + 1]) + D[i] + D[i + 1]);
    }

    return C;
}

void TransferFunction::drawControlPointCreationUi()
{
    // creation bar
    static int pointType = 0;
    static int isoValue = 127;
    static float alpha;
    static vec3 color;
    ui::BeginGroup();
    ui::Separator();
    ui::RadioButton("Alpha Point", &pointType, 0);
    ui::SameLine();
    ui::RadioButton("Color Point", &pointType, 1);
    // iso value bar
    ui::SliderInt("Iso Value", &isoValue, 1, 254);

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
    ui::SameLine();

    if (ui::Button("Add Point", ImVec2(ui::GetContentRegionAvailWidth(), size.y)))
    {
        pointType == 0 ? addAlphaPoint(alpha, isoValue) : addColorPoint(color, isoValue);
    }

    drawControlPointList(pointType);
}

void TransferFunction::drawControlPointsUi()
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

    for (int i = 0; i < 256; i++)
    {
        const auto& color = getColor(static_cast<float>(i) / 255);
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

    for (int i = 0; i < indexedTransferFunction.size() - 1; i++)
    {
        auto aBegin = clamp(1.0f - indexedTransferFunction[i].a, 0.0f, 1.0f);
        auto aEnd = clamp(1.0f - indexedTransferFunction[i + 1].a, 0.0f, 1.0f);
        drawList->AddLine(ImVec2(p.x + step * i + 4.0f, y - 124.0f + aBegin * 120),
                          ImVec2(p.x + step * (i + 1) + 4.0f, y - 124.0f + aEnd * 120), white, 1);
    }

    for (auto& alphaP : alphaPoints)
    {
        const auto& alpha = 1.0f - alphaP.getAlpha();
        ImU32 opacity = ImColor(1.0f - vec4(alpha));
        drawList->AddCircleFilled(ImVec2(p.x + step * alphaP.getIsoValue() + 4.0f, y - 124.0f + alpha * 120), 5, opacity, 24);
        drawList->AddCircle(ImVec2(p.x + step * alphaP.getIsoValue() + 4.0f, y - 124.0f + alpha * 120), 5, black, 24);
    }

    ui::Dummy(ImVec2(ui::GetContentRegionAvailWidth(), sz + 15.0f));
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
    if (!ui::Begin("Transfer Function", &open, ImGuiWindowFlags_AlwaysAutoResize))
    {
        ui::End();
    }

    if (volume != nullptr)
    {
        drawHistogram();
        drawControlPointsUi();
        drawControlPointCreationUi();
    }

    ui::End();
}

void TransferFunction::addColorPoint(const vec3& color, const int isoValue)
{
    // inserted point is off limits
    if (isoValue <= 0 || isoValue >= colorPoints.back().getIsoValue())
    {
        return;
    }

    auto ctrlP = TransferFunctionColorPoint(color, isoValue);
    // sorted insert
    colorPoints.insert(upper_bound(colorPoints.begin(), colorPoints.end(), ctrlP), ctrlP);
    updateSplines();
}

void TransferFunction::addAlphaPoint(const float alpha, const int isoValue)
{
    // inserted point is off limits
    if (isoValue <= 0 || isoValue >= alphaPoints.back().getIsoValue())
    {
        return;
    }

    auto ctrlP = TransferFunctionAlphaPoint(alpha, isoValue);
    // sorted insert
    alphaPoints.insert(upper_bound(alphaPoints.begin(), alphaPoints.end(), ctrlP), ctrlP);
    updateSplines();
}

void TransferFunction::removeColorPoint(const int isoValue)
{
    // off limits
    if (isoValue <= 0 || isoValue >= colorPoints.back().getIsoValue())
    {
        return;
    }

    colorPoints.erase(std::remove_if(colorPoints.begin(), colorPoints.end(),
                                     [=](const TransferFunctionColorPoint& p)
                                     {
                                         return p.getIsoValue() == isoValue;
                                     }));
    updateSplines();
}

void TransferFunction::removeAlphaPoint(const int isoValue)
{
    // off limits
    if (isoValue <= 0 || isoValue >= alphaPoints.back().getIsoValue())
    {
        return;
    }

    alphaPoints.erase(std::remove_if(alphaPoints.begin(), alphaPoints.end(),
                                     [=](const TransferFunctionAlphaPoint& p)
                                     {
                                         return p.getIsoValue() == isoValue;
                                     }));
    updateSplines();
}

TransferFunction::Cubic::Cubic(vec3 a, vec3 b, vec3 c, vec3 d)
{
    this->a = a;
    this->b = b;
    this->c = c;
    this->d = d;
}

vec3 TransferFunction::Cubic::getPointOnSpline(float s) const
{
    return (((d * s) + c) * s + b) * s + a;
}

void TransferFunction::setVolume(const Volume3D& volume)
{
    this->volume = &volume;
}

void TransferFunction::updateSplines()
{
    std::vector<vec3> alphaCtrPoints;
    std::vector<vec3> colorCtrPoints;

    for (auto& val : alphaPoints)
    {
        alphaCtrPoints.push_back(vec3(val.getAlpha()));
    }

    for (auto& val : colorPoints)
    {
        colorCtrPoints.push_back(vec3(val.getColor()));
    }

    alphaSpline = calculateCubicSpline(alphaCtrPoints);
    colorSpline = calculateCubicSpline(colorCtrPoints);

    for (int i = 0; i < indexedTransferFunction.size(); i++)
    {
        indexedTransferFunction[i] = getColor(static_cast<float>(i) / 255);
    }
}

vec4 TransferFunction::getColor(const float t)
{
    float alpha = 0;
    vec3 color = vec3(0);
    int i = 0;
    int isoValue = t * 255;

    for (auto it = alphaPoints.cbegin(); it != --alphaPoints.cend() && i < alphaSpline.size();)
    {
        int currentIso = it->getIsoValue();
        int nextIso = (++it)->getIsoValue();

        // find cubic index
        if (isoValue >= currentIso && isoValue <= nextIso)
        {
            auto& currentCubic = alphaSpline[i];
            float evalAt = static_cast<float>(isoValue - currentIso) / (nextIso - currentIso);
            alpha = currentCubic.getPointOnSpline(evalAt).r;
            break;
        }

        i++;
    }

    i = 0;

    for (auto it = colorPoints.cbegin(); it != --colorPoints.cend() && i < colorSpline.size();)
    {
        int currentIso = it->getIsoValue();
        int nextIso = (++it)->getIsoValue();

        // find cubic index
        if (isoValue >= currentIso && isoValue <= nextIso)
        {
            auto& currentCubic = colorSpline[i];
            float evalAt = static_cast<float>(isoValue - currentIso) / (nextIso - currentIso);
            color = currentCubic.getPointOnSpline(evalAt);
            break;
        }

        i++;
    }

    return vec4(color.r, color.g, color.b, alpha);
}
