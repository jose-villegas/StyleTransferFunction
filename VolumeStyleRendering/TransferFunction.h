#pragma once
#include "TransferFunctionPoint.h"

class Volume3D;

class TransferFunction
{
public:
    TransferFunction();
    explicit TransferFunction(const int limit);
    ~TransferFunction();
    void drawUi(bool& open);
    void setVolume(const Volume3D& volume);
    void setLimit(const int limit);
    void addColorPoint(const glm::vec3& color, const int isoValue);
    void addAlphaPoint(const float alpha, const int isoValue);
    void removeColorPoint(const int isoValue);
    void removeAlphaPoint(const int isoValue);
private:
    const Volume3D* volume;
    std::map<int, TransferFunctionColorPoint> colorPoints;
    std::map<int, TransferFunctionAlphaPoint> alphaPoints;
    void insertLimitPoints(const int limit);
    void drawHistogram();
    void drawColorPointsUi();
    void drawControlPointCreationUi();
    void drawControlPointList(int& pointType);
};
