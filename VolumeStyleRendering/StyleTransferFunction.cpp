#include "StyleTransferFunction.h"
using namespace glm;
std::vector<Style> Style::styles(0);

StylePoint::StylePoint(): styleIndex(-1) {}

StylePoint::StylePoint(int isoValue, unsigned styleIndex): TransferFunctionPoint(isoValue),
                                                           styleIndex(styleIndex) {}

int StylePoint::getStyleIndex() const { return styleIndex; }

const Style &StylePoint::getStyle() const
{
    if (styleIndex < 0 || styleIndex >= Style::GetAvailableStyles().size()) { return Style(); }

    return Style::GetAvailableStyles()[styleIndex];
}

void StylePoint::setStyle(unsigned index)
{
    if (Style::GetAvailableStyles().empty()) { styleIndex = -1; }

    styleIndex = min(max(index, 0U), Style::GetAvailableStyles().size());
}

void Style::AddStyle(const Style& style)
{
    styles.push_back(style);
    styles.back().name = style.name.substr(0, 32);
}

void Style::RenameStyle(const int index, const std::string& name)
{
    if (index < 0 || index >= styles.size() || name.empty())
    {
        return;
    }

    styles[index].name = name.substr(0, 32);
}

const std::vector<Style> &Style::GetAvailableStyles()
{
    return styles;
}

StyleTransferFunction::StyleTransferFunction() {}

StyleTransferFunction::~StyleTransferFunction() {}

void StyleTransferFunction::addStylePoint(const StylePoint& style)
{
    if (style.getIsoValue() <= 0 || style.getIsoValue() >= 255 || stylePoints.size() > 255)
    {
        return;
    }

    // sorted insert
    stylePoints.insert(upper_bound(stylePoints.begin(), stylePoints.end(), style), style);
    updateFunction();
}

void StyleTransferFunction::removeStylePoint(const int index)
{
    // off limits
    if (index < 0 || index >= stylePoints.size())
    {
        return;
    }

    stylePoints.erase(stylePoints.begin() + index);
    updateFunction();
}

void StyleTransferFunction::setStylePoint(const int index, const int styleIndex)
{
    // off limits
    if (index < 0 || index >= stylePoints.size())
    {
        return;
    }

    stylePoints[index].setStyle(clamp(styleIndex, 0, static_cast<int>(Style::GetAvailableStyles().size()) - 1));
    updateFunction();
}

void StyleTransferFunction::updateFunction()
{
    // call base method to update splines
    TransferFunction::updateFunction();

    // build style transfer function data
    std::vector<vec2> tft(256); // transfer function
    std::vector<int> ift; // index function
    // first point no texture
    ift.push_back(-1);

    for (auto& p : stylePoints) { ift.push_back(p.getStyleIndex()); }
    for (int i = 0; i < tft.size(); i++) { tft[i].y = getColor(i).a; }

    if (!stylePoints.empty())
    {
        int limit = stylePoints.front().getIsoValue();
        // zero to first control point iso value linear interpolation
        for (int i = 0; i < limit; i++)
        {
            float start = 0.0f;
            float end = 1.0f;
            float t = static_cast<float>(i) / limit;
            tft[i].x = lerp(start, end, t);
        }

        for (int j = 0; j < stylePoints.size() - 1; j++)
        {
            int limitStart = stylePoints[j].getIsoValue();
            int limitEnd = stylePoints[j + 1].getIsoValue();

            for(int i = limitStart; i < limitEnd; i++)
            {
                float start = j + 1;
                float end = j + 2;
                float t = static_cast<float>(i - limitStart) / (limitEnd - limitStart);
                tft[i].x = lerp(start, end, t);
            }
        }

        limit = stylePoints.back().getIsoValue();
        // last control point to last iso value linear interpolation
        for (int i = limit; i < 256; i++)
        {
            float start = stylePoints.size();
            float end = stylePoints.size() + 1;
            float t = (float)(i - limit) / (255 - limit);
            tft[i].x = lerp(start, end, t);
        }
    }

    // last point
    ift.push_back(-1);
}

const std::vector<StylePoint> &StyleTransferFunction::getStylePoints() const
{
    return stylePoints;
}

void StyleTransferFunction::setStylePointIsoValue(const int index, const int isoValue)
{
    // off limits
    if (index < 0 || index >= stylePoints.size()) { return; }

    // off limits iso val
    if (isoValue <= 0 || isoValue >= 255) { return; }

    stylePoints[index].setIsoValue(isoValue);

    // function points may need sorting after modiying sorting token, isoValue
    bool needSorting = !is_sorted(stylePoints.begin(), stylePoints.end());

    if (needSorting) { sort(stylePoints.begin(), stylePoints.end()); }

    updateFunction();
}
