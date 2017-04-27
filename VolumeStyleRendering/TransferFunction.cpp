#include "TransferFunction.h"

#include "CinderImGui.h"
using namespace glm;

TransferFunction::TransferFunction(const int limit)
{
    colorPoints[0] = TransferFunctionColorPoint(vec3(1), 0);
    colorPoints[limit] = TransferFunctionColorPoint(vec3(1), limit);
  
    alphaPoints[0]= TransferFunctionAlphaPoint(1.0, 0);
    alphaPoints[limit] = TransferFunctionAlphaPoint(1.0, limit);
}

TransferFunction::TransferFunction()
{
    TransferFunction(std::numeric_limits<uint8_t>().max());
}

TransferFunction::~TransferFunction() {}

void TransferFunction::drawUi(bool& open)
{
    ui::ShowTestWindow();

    if(!ui::Begin("Transfer Function", &open))
    {
        ui::End();
    }
}

void TransferFunction::setLimit(const int limit)
{
    alphaPoints.end()->second.setIsoValue(limit);
    colorPoints.end()->second.setIsoValue(limit);
}

void TransferFunction::addColorPoint(const vec3 &color, const int isoValue)
{
    // inserted point is off limits
    if(isoValue <= 0 || isoValue >= colorPoints.end()->second.getIsoValue())
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

    if(it != colorPoints.end())
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
