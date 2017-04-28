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

    updateSplines();
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
                updateSplines();
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
                        updateSplines();
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
                    updateSplines();
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
                updateSplines();
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
                        updateSplines();
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
                    updateSplines();
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

        updateSplines();
    }

    drawControlPointList(pointType);
}

void TransferFunction::drawColorPointsUi()
{
    const ImVec2 p = ui::GetCursorScreenPos();
    ImDrawList* drawList = ui::GetWindowDrawList();
    float sz = 20.0f;
    float x = p.x + 4.0f;
    float y = p.y + 4.0f;

    for (int i = 0; i < 256; i++)
    {
        const auto &color = getColor(static_cast<float>(i) / 255);
        ImU32 col32 = ImColor(color);

        drawList->AddLine(ImVec2(x, y), ImVec2(x, y + sz), col32, 4);
        x += 2.0f;
    }

    x = p.x + 4.0f;

    for(auto &colorP : colorPoints)
    {
        const auto &color = colorP.second.getColor();
        ImU32 col32 = ImColor(color.r, color.g, color.b, 1.0f);
        ImU32 invCol32 = ImColor(1.0f - color.r, 1.0f - color.g, 1.0f - color.b, 1.0f);

        drawList->AddCircleFilled(ImVec2(p.x + 2.0f * colorP.first + 4.0f, y + sz), 5, col32, 24);
        drawList->AddCircle(ImVec2(p.x + 2.0f * colorP.first + 4.0f, y + sz), 5, invCol32, 24);
    }

    ui::Dummy(ImVec2(520, sz + 12.0f));
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
        alphaCtrPoints.push_back(vec3(val.second.getAlpha()));
    }

    for (auto& val : colorPoints)
    {
        colorCtrPoints.push_back(vec3(val.second.getColor()));
    }

    alphaSpline = calculateCubicSpline(alphaCtrPoints);
    colorSpline = calculateCubicSpline(colorCtrPoints);
}

vec4 TransferFunction::getColor(const float t)
{
    float alpha = 0;
    vec3 color = vec3(0);
    int i = 0;
    int isoValue = t * 255;

    for (auto it = alphaPoints.cbegin(); it != --alphaPoints.cend() && i < alphaSpline.size();)
    {
        int currentIso = it->first;
        int nextIso = (++it)->first;

        // find cubic index
        if(isoValue >= currentIso && isoValue < nextIso)
        {
            auto &currentCubic = alphaSpline[i];
            float evalAt = static_cast<float>(isoValue - currentIso) / (nextIso - currentIso);
            alpha = currentCubic.getPointOnSpline(evalAt).r;
            break;
        }
 
        i++;
    }

    i = 0;

    for (auto it = colorPoints.cbegin(); it != --colorPoints.cend() && i < colorSpline.size();)
    {
        int currentIso = it->first;
        int nextIso = (++it)->first;

        // find cubic index
        if (isoValue >= currentIso && isoValue < nextIso)
        {
            auto &currentCubic = colorSpline[i];
            float evalAt = static_cast<float>(isoValue - currentIso) / (nextIso - currentIso);
            color = currentCubic.getPointOnSpline(evalAt);
            break;
        }

        i++;
    }

    return vec4(color.r, color.g, color.b, alpha);
}
