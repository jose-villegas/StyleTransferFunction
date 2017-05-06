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
    void drawThresholdControl();
    void drawControlPointsUi() const;
    void drawControlPointList(int pointType);
    void drawControlPointCreationUi();
};
