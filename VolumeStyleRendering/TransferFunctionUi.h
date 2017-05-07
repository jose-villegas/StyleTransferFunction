#pragma once
#include "StyleTransferFunction.h"
class RaycastVolume;

class TransferFunctionUi
{
public:
    void drawUi(bool& open, const RaycastVolume& volume);
    TransferFunctionUi();
    ~TransferFunctionUi();

    const std::shared_ptr<StyleTransferFunction> &getTranferFunction() const;
private:
    std::shared_ptr<StyleTransferFunction> transferFunction;
    void drawHistogram(const RaycastVolume& volume) const;

    int stylesManagerPopup() const;
    void drawThresholdControl() const;
    void drawControlPointsUi() const;
    void drawPointIsoValueControl(const TransferFunctionPoint& p, const int index, int pointType) const;
    void drawAlphaPointList() const;
    void drawColorPointList() const;
    void drawStylePointList() const;
    void drawControlPointList(int pointType) const;
    void drawControlPointCreationUi() const;
};
