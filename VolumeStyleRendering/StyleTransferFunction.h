#pragma once
#include "cinder/gl/gl.h"
#include "TransferFunction.h"

class Style
{
public:
    std::string name;
    ci::gl::Texture2dRef litsphere;
    std::string filepath;

    static void AddStyle(const Style& style);
    static void RenameStyle(const int index, const std::string& name);
    static const std::vector<Style> &GetAvailableStyles();
private:
    static std::vector<Style> styles;
};

class StylePoint : public TransferFunctionPoint
{
public:
    StylePoint();
    StylePoint(int isoValue, unsigned styleIndex);
    int getStyle() const;
    void setStyle(unsigned index);
private:
    unsigned styleIndex;
};

class StyleTransferFunction : public TransferFunction
{
public:
    StyleTransferFunction();
    ~StyleTransferFunction();

    void addStylePoint(const StylePoint& style);
    void removeStylePoint(const int isoValue);
    void updateFunction() override;
private:
    std::vector<StylePoint> stylePoints;
    std::vector<TransferFunctionAlphaPoint> alphaPoints;
    std::vector<CubicSpline> alphaSpline;

    ci::gl::Texture1dRef transferFunctionTexture;
    ci::gl::Texture1dRef indexFunctionTexture;
    ci::gl::Texture3dRef styleFunctionTexture;
};
