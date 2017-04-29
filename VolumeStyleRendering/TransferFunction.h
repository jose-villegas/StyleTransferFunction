#pragma once
#include "TransferFunctionPoint.h"

class Volume3D;

class TransferFunction
{
public:
    TransferFunction();
    ~TransferFunction();
    glm::vec4 getColor(const float t /* t=[0..1] */);
    void drawUi(bool& open);
    void setVolume(const Volume3D& volume);
    void updateSplines();
    void addColorPoint(const glm::vec3& color, const int isoValue);
    void addAlphaPoint(const float alpha, const int isoValue);
    void removeColorPoint(const int isoValue);
    void removeAlphaPoint(const int isoValue);
private:
    class Cubic
    {
    public:
        Cubic() = default;
        Cubic(glm::vec3 a, glm::vec3 b, glm::vec3 c, glm::vec3 d);
        //evaluate the point using a cubic equation
        glm::vec3 getPointOnSpline(float s) const;
    private:
        glm::vec3 a;
        glm::vec3 b;
        glm::vec3 c;
        glm::vec3 d;
    };

    const Volume3D* volume;
    std::vector<TransferFunctionColorPoint> colorPoints;
    std::vector<TransferFunctionAlphaPoint> alphaPoints;
    std::vector<Cubic> alphaSpline;
    std::vector<Cubic> colorSpline;
    std::array<glm::vec4, 256> indexedTransferFunction;

    void insertLimitPoints(const int limit);
    void drawHistogram();
    void drawControlPointsUi();
    void drawControlPointCreationUi();
    void drawControlPointList(int& pointType);

    static std::vector<Cubic> calculateCubicSpline(std::vector<glm::vec3> points);
};
