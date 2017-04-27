#pragma once
#include "TransferFunctionPoint.h"

class TransferFunction
{
public:
    TransferFunction();
    explicit TransferFunction(const int limit);
    ~TransferFunction();
    void drawUi(bool &open);
    void setLimit(const int limit);
    void addColorPoint(const glm::vec3 &color, const int isoValue);
    void addAlphaPoint(const float alpha, const int isoValue);
    void removeColorPoint(const int isoValue);
    void removeAlphaPoint(const int isoValue);
private:
    std::map<int, TransferFunctionColorPoint> colorPoints;
    std::map<int, TransferFunctionAlphaPoint> alphaPoints;
};
