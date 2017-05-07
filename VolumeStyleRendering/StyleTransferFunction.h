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
    int getStyleIndex() const;
    const Style &getStyle() const;
    void setStyle(unsigned index);
private:
    int styleIndex;
};

class StyleTransferFunction : public TransferFunction
{
public:
    StyleTransferFunction();
    ~StyleTransferFunction();

    void addStylePoint(const StylePoint& style);
    void removeStylePoint(const int index);
    void setStylePoint(const int index, const int styleIndex);
    void updateFunction() override;
    const std::vector<StylePoint> &getStylePoints() const;
    void setStylePointIsoValue(const int index, const int isoValue);;
private:
    std::vector<StylePoint> stylePoints;

    ci::gl::Texture1dRef transferFunctionTexture;
    ci::gl::Texture1dRef indexFunctionTexture;
    ci::gl::Texture3dRef styleFunctionTexture;
};
