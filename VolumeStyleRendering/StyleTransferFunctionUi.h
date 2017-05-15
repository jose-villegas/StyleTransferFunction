#pragma once
#include <cinder/Json.h>

#include "StyleTransferFunction.h"

class RaycastVolume;

class StyleTransferFunctionUi
{
public:
    void drawUi(bool& open, const RaycastVolume& volume);
    StyleTransferFunctionUi();
    ~StyleTransferFunctionUi();

    const std::shared_ptr<StyleTransferFunction> &getTranferFunction() const;
private:
    std::vector<std::pair<std::string, ci::JsonTree>> savedTransferFunctions;
    std::shared_ptr<StyleTransferFunction> transferFunction;
    bool showTFManager;
    void drawHistogram(const RaycastVolume& volume) const;
    ci::JsonTree buildTransferFunctionJSON() const;
    void loadTransferFunctionJSON(const cinder::JsonTree& second) const;

    int stylesManagerPopup() const;
    void drawThresholdControl() const;
    void drawControlPointsUi() const;
    void drawPointIsoValueControl(const TransferFunctionPoint& p, const int index, int pointType) const;
    void drawAlphaPointList() const;
    void drawColorPointList() const;
    void drawStylePointList() const;
    void drawControlPointList(int pointType) const;
    void drawTransferFunctionsManager();
    void drawControlPointCreationUi();
};
