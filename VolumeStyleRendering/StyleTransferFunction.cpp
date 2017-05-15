#include "StyleTransferFunction.h"
#include <cinder/ip/Resize.h>
#include <cinder/app/AppBase.h>
using namespace glm;
using namespace ci;
using namespace app;
std::vector<Style> Style::styles(0);

StylePoint::StylePoint(): styleIndex(-1) {}

StylePoint::StylePoint(int isoValue, unsigned styleIndex): TransferFunctionPoint(isoValue),
                                                           styleIndex(styleIndex) {}

int StylePoint::getStyleIndex() const { return styleIndex; }

const Style& StylePoint::getStyle() const
{
    if (styleIndex < 0 || styleIndex >= Style::GetAvailableStyles().size()) { return Style::GetDefaultStyle(); }

    return Style::GetAvailableStyles()[styleIndex];
}

void StylePoint::setStyle(unsigned int index)
{
    if (Style::GetAvailableStyles().empty()) { styleIndex = -1; }

    styleIndex = clamp(static_cast<int>(index), 0, static_cast<int>(Style::GetAvailableStyles().size()) - 1);
}

void Style::AddStyle(const std::string& name, const std::string& filepath)
{
    // max number of styles
    if (styles.size() > 127) { return; }

    // check if filepath has been already loaded
    for (auto& s : styles) { if (s.filepath == filepath) return; }

    // resize image to fit within 2d array of textures
    Surface baseImage = loadImage(filepath);
    Surface resizedImage(512, 512, true, SurfaceChannelOrder::RGBA);
    ip::resize(baseImage, &resizedImage);
    auto texture = gl::Texture2d::create(resizedImage, gl::Texture2d::Format()
                                         .wrap(GL_CLAMP_TO_BORDER)
                                         .minFilter(GL_LINEAR)
                                         .magFilter(GL_LINEAR)
                                         .internalFormat(GL_RGBA8));
    auto style = Style(name, resizedImage, texture, filepath);

    styles.push_back(style);
    styles.back().name = style.name.substr(0, 32);
}

void Style::RemoveStyle(const int index)
{
    if (index <= 0 || index >= styles.size()) { return; }

    styles.erase(styles.begin() + index);
}

void Style::RenameStyle(const int index, const std::string& name)
{
    if (index <= 0 || index >= styles.size() || name.empty()) { return; }

    styles[index].name = name.substr(0, 32);
}

const std::vector<Style>& Style::GetAvailableStyles()
{
    if (styles.empty()) { GetDefaultStyle(); }

    return styles;
}

StyleTransferFunction::StyleTransferFunction(): textureDataChanged(false)
{
    transferFunction.resize(256);
    // transfer function texture has a fixed size initialize at once
    transferFunctionTexture = gl::Texture1d::create(256, gl::Texture1d::Format()
                                                    .internalFormat(GL_RG16F)
                                                    .wrap(GL_REPEAT)
                                                    .minFilter(GL_LINEAR)
                                                    .magFilter(GL_LINEAR));
    // styles function texture has a fixed max size initialize at once
    auto stylesFormat = gl::Texture3d::Format().target(GL_TEXTURE_2D_ARRAY)
                                               .magFilter(GL_LINEAR)
                                               .minFilter(GL_LINEAR)
                                               .wrap(GL_CLAMP_TO_EDGE)
                                               .internalFormat(GL_RGBA);
    styleFunctionTexture = gl::Texture3d::create(512, 512, 128, stylesFormat);
}

StyleTransferFunction::~StyleTransferFunction() {}

void StyleTransferFunction::addStylePoint(const StylePoint& style)
{
    if (style.getIsoValue() <= 0 || style.getIsoValue() >= 255 || stylePoints.size() > 255) { return; }

    // sorted insert
    stylePoints.insert(upper_bound(stylePoints.begin(), stylePoints.end(), style), style);
    updateFunction();
}

void StyleTransferFunction::removeStylePoint(const int index)
{
    // off limits
    if (index < 0 || index >= stylePoints.size()) { return; }

    stylePoints.erase(stylePoints.begin() + index);
    updateFunction();
}

void StyleTransferFunction::setStylePoint(const int index, const int styleIndex)
{
    // off limits
    if (index < 0 || index >= stylePoints.size()) { return; }

    stylePoints[index].setStyle(clamp(styleIndex, 0, static_cast<int>(Style::GetAvailableStyles().size()) - 1));
    updateFunction();
}

void StyleTransferFunction::updateFunction()
{
    // call base method to update splines
    TransferFunction::updateFunction();

    // build style transfer function data
    indexFunction.clear();
    // first point no texture
    indexFunction.push_back(-1);

    for (auto& p : stylePoints) { indexFunction.push_back(p.getStyleIndex()); }
    for (int i = 0; i < transferFunction.size(); i++) { transferFunction[i].y = getColor(i).a; }

    if (!stylePoints.empty())
    {
        int limit = stylePoints.front().getIsoValue();
        // zero to first control point iso value linear interpolation
        for (int i = 0; i < limit; i++)
        {
            float start = 0.0f;
            float end = 1.0f;
            float t = static_cast<float>(i) / limit;
            transferFunction[i].x = lerp(start, end, t);
        }

        for (int j = 0; j < stylePoints.size() - 1; j++)
        {
            int limitStart = stylePoints[j].getIsoValue();
            int limitEnd = stylePoints[j + 1].getIsoValue();

            for (int i = limitStart; i < limitEnd; i++)
            {
                float start = j + 1;
                float end = j + 2;
                float t = static_cast<float>(i - limitStart) / (limitEnd - limitStart);
                transferFunction[i].x = lerp(start, end, t);
            }
        }

        limit = stylePoints.back().getIsoValue();
        // last control point to last iso value linear interpolation
        for (int i = limit; i < 256; i++)
        {
            float start = stylePoints.size();
            float end = stylePoints.size() + 1;
            float t = (float)(i - limit) / (255 - limit);
            transferFunction[i].x = lerp(start, end, t);
        }
    }

    // last point
    indexFunction.push_back(-1);
    // needs to update textures 
    textureDataChanged = true;
}

const std::vector<StylePoint>& StyleTransferFunction::getStylePoints() const
{
    return stylePoints;
}

void StyleTransferFunction::setStylePointIsoValue(const int index, const int isoValue)
{
    // off limits
    if (index < 0 || index >= stylePoints.size()) { return; }

    // off limits iso val
    if (isoValue <= 0 || isoValue >= 255) { return; }

    stylePoints[index].setIsoValue(isoValue);

    // function points may need sorting after modiying sorting token, isoValue
    bool needSorting = !is_sorted(stylePoints.begin(), stylePoints.end());

    if (needSorting) { sort(stylePoints.begin(), stylePoints.end()); }

    updateFunction();
}

void StyleTransferFunction::updateTextures()
{
    // update tft with new data
    transferFunctionTexture->update(transferFunction.data(), GL_RG, GL_FLOAT, 0, 256, 0);

    // create ift according to size
    static auto iftFormat = gl::Texture1d::Format().minFilter(GL_NEAREST)
                                                   .magFilter(GL_NEAREST)
                                                   .wrap(GL_CLAMP_TO_BORDER)
                                                   .internalFormat(GL_R32I);
    iftFormat.setDataType(GL_INT);
    indexFunctionTexture = gl::Texture1d::create(indexFunction.data(), GL_RED_INTEGER, indexFunction.size(),
                                                 iftFormat);

    // update styles function
    int index = 0;

    for (auto& style : Style::GetAvailableStyles())
    {
        styleFunctionTexture->update(style.getSurface().getData(), GL_RGBA, GL_UNSIGNED_BYTE, 0, 512, 512, 1,
                                     0, 0, index++);
    }

    textureDataChanged = false;
}

const gl::Texture1dRef& StyleTransferFunction::getTransferFunctionTexture()
{
    if (textureDataChanged)
    {
        updateTextures();
    }

    return transferFunctionTexture;
}

const gl::Texture1dRef& StyleTransferFunction::getIndexFunctionTexture()
{
    if (textureDataChanged)
    {
        updateTextures();
    }

    return indexFunctionTexture;
}

const gl::Texture3dRef& StyleTransferFunction::getStyleFunctionTexture()
{
    if (textureDataChanged)
    {
        updateTextures();
    }

    return styleFunctionTexture;
}

void StyleTransferFunction::reset()
{
    stylePoints.clear();
    TransferFunction::reset();
}

const Style& Style::GetDefaultStyle()
{
    if (styles.empty())
    {
        Surface baseImage = loadImage(loadAsset("images/default.png"));
        Surface resizedImage(512, 512, true, SurfaceChannelOrder::RGBA);
        ip::resize(baseImage, &resizedImage);
        auto texture = gl::Texture2d::create(resizedImage);
        styles.push_back(Style("Default", resizedImage, texture, getAssetPath("images/default.png").string()));

        return styles.back();
    }

    return styles[0];
}
