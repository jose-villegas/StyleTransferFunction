#pragma once
#include <cinder/gl/gl.h>
#include "TransferFunction.h"

class Style
{
public:
    static void AddStyle(const Style& style);
    static void RemoveStyle(const int index);
    static void RenameStyle(const int index, const std::string& name);
    static const std::vector<Style> &GetAvailableStyles();
    static const Style &GetDefaultStyle();

    const std::string &getName() const { return name; }
    const std::string &getFilepath() const { return filepath; }
    const ci::gl::Texture2dRef &getTexture() const { return litsphereTexture; }
    const ci::Surface &getSurface() const { return litsphere; }

    Style() = default;
    Style(const std::string &name, const ci::Surface &surface, const ci::gl::Texture2dRef &texture, const std::string &filepath)
    {
        this->name = name;
        this->litsphere = surface;
        this->litsphereTexture = texture;
        this->filepath = filepath;
    }
private:
    std::string name;
    ci::Surface litsphere;
    ci::gl::Texture2dRef litsphereTexture;
    std::string filepath;
    static std::vector<Style> styles;
};

class StylePoint : public TransferFunctionPoint
{
public:
    StylePoint();
    StylePoint(int isoValue, unsigned styleIndex);
    int getStyleIndex() const;
    const Style &getStyle() const;
    void setStyle(unsigned int index);
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
    void setStylePointIsoValue(const int index, const int isoValue);
    void updateTextures();
    const ci::gl::Texture1dRef &getTransferFunctionTexture();
    const ci::gl::Texture1dRef &getIndexFunctionTexture();
    const ci::gl::Texture3dRef &getStyleFunctionTexture();
    void reset() override;
private:
    std::vector<StylePoint> stylePoints;
    std::vector<glm::vec2> transferFunction;
    std::vector<int> indexFunction;
    bool textureDataChanged;

    ci::gl::Texture1dRef transferFunctionTexture;
    ci::gl::Texture1dRef indexFunctionTexture;
    ci::gl::Texture3dRef styleFunctionTexture;
};
