#include "StyleTransferFunction.h"
using namespace glm;
std::vector<Style> Style::styles(0);

StylePoint::StylePoint(): styleIndex(0) {}

StylePoint::StylePoint(int isoValue, unsigned styleIndex): TransferFunctionPoint(isoValue),
                                                           styleIndex(styleIndex) {}

int StylePoint::getStyle() const { return styleIndex; }

void StylePoint::setStyle(unsigned index)
{
    styleIndex = min(max(index, 0U), Style::GetAvailableStyles().size());
}

void Style::AddStyle(const Style& style)
{
    styles.push_back(style);
}

void Style::RenameStyle(const int index, const std::string& name)
{
    if(index < 0 || index >= styles.size() || name.empty())
    {
        return;
    }

    styles[index].name = name;
}

const std::vector<Style>& Style::GetAvailableStyles()
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
}

void StyleTransferFunction::removeStylePoint(const int isoValue)
{
    // off limits
    if (isoValue <= 0 || isoValue >= 255)
    {
        return;
    }

    stylePoints.erase(std::remove_if(stylePoints.begin(), stylePoints.end(),
                                     [=](const StylePoint& p)
                                     {
                                         return p.getIsoValue() == isoValue;
                                     }));
}

void StyleTransferFunction::updateFunction()
{
    // call base method to update splines
    TransferFunction::updateFunction();

    // build style transfer function data
    std::vector<vec2> tft(256); // transfer function
    std::vector<uint8_t> ift(stylePoints.size()); // index function
    int styleAt = 0;

    for (auto &p : stylePoints) { ift[styleAt++] = p.getStyle(); }

    for(auto i = 0; i < stylePoints.size() - 1 && stylePoints.size() > 0; i++)
    {
        auto &start = stylePoints[i];
        auto &end = stylePoints[i +1];

        for (int j = start.getIsoValue(); j <= end.getIsoValue(); j++)
        {
            tft[j].x = lerp(static_cast<float>(start.getStyle()),
                static_cast<float>(end.getStyle()),
                static_cast<float>(j) / end.getIsoValue());
            tft[j].y = getColor(j).a;
        }
    }
}
