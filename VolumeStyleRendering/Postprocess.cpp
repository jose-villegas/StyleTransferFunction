#include "PostProcess.h"
#include <cinder/app/AppBase.h>
#include <cinder/Log.h>
#include "RenderingParams.h"
using namespace ci;
using namespace glm;
using namespace app;

void PostProcess::displayTexture(const gl::Texture2dRef colorTex) const
{
    const gl::ScopedTextureBind scopedTextureBind(colorTex ? colorTex : getColorTexture(), 0);
    const gl::ScopedViewport scopedViewport(ivec2(0), toPixels(getWindowSize()));
    const gl::ScopedMatrices scopedMatrices;
    const gl::ScopedDepth depth(false);
    gl::clear();

    // translate to center and scale to fit whole screen
    gl::translate(getWindowCenter());
    gl::scale(getWindowSize());

    // draw quad with mapped texture
    textureRect->draw();
}

void PostProcess::displayFXAA(const gl::Texture2dRef& texture) const
{
    const gl::ScopedTextureBind scopedTextureBind(texture ? texture : getColorTexture(), 0);
    const gl::ScopedViewport scopedViewport(ivec2(0), toPixels(getWindowSize()));
    const gl::ScopedMatrices scopedMatrices;
    const gl::ScopedDepth depth(false);
    gl::clear();

    // custom uniforms
    auto& prog = fxaaRect->getGlslProg();
    prog->uniform("pixelSize", vec2(1.0f) / vec2(toPixels(getWindowSize())));

    // translate to center and scale to fit whole screen
    gl::translate(getWindowCenter());
    gl::scale(getWindowSize());

    // draw quad with mapped texture
    fxaaRect->draw();
}

void PostProcess::toneMapping(const gl::Texture2dRef& hdrBuffer, bool local) const
{
    if (local) Start();

    const gl::ScopedTextureBind scopedTextureBind(hdrBuffer ? hdrBuffer : getColorTexture(), 0);

    // custom uniforms
    auto& prog = toneMappingRect->getGlslProg();
    prog->uniform("gamma", RenderingParams::GetGamma());
    prog->uniform("exposure", RenderingParams::GetExposure());

    // draw quad with mapped texture
    toneMappingRect->draw();

    if (local) End();
}

void PostProcess::blurHorizontal(const gl::Texture2dRef& texture, bool local) const
{
    if (local) Start();

    const gl::ScopedTextureBind scopedTextureBind(texture ? texture : getColorTexture(), 0);

    // custom uniforms
    auto& prog = blurRect->getGlslProg();
    prog->uniform("blurDirection", vec2(1.0f / static_cast<float>(currentFbo->getSize().x), 0.0f));

    // first pass blur
    blurRect->draw();

    if (local) End();
}

void PostProcess::blurVertical(const gl::Texture2dRef& texture, bool local) const
{
    if (local) Start();

    const gl::ScopedTextureBind scopedTextureBind(texture ? texture : getColorTexture(), 0);

    // custom uniforms
    auto& prog = blurRect->getGlslProg();
    prog->uniform("blurDirection", vec2(0.0f, 1.0f / static_cast<float>(currentFbo->getSize().y)));

    // first pass blur
    blurRect->draw();

    if (local) End();
}

void PostProcess::inverse(const gl::Texture2dRef& texture, bool local ) const
{
    if (local) Start();

    const gl::ScopedTextureBind scopedTextureBind(texture ? texture : getColorTexture(), 0);

    // draw quad with mapped texture
    inverseRect->draw();

    if (local) End();
}

void PostProcess::multiply(const gl::Texture2dRef& texture0, const gl::Texture2dRef& texture1, bool local) const
{
    if (local) Start();

    const gl::ScopedTextureBind scopedTextureBind0(texture0, 0);
    const gl::ScopedTextureBind scopedTextureBind1(texture1 ? texture1 : getColorTexture(), 1);

    // draw quad with mapped texture
    multiplyRect->draw();

    if (local) End();
}

void PostProcess::Start()
{
    gl::context()->pushFramebuffer(instance().currentFbo);
    gl::context()->pushViewport({ivec2(0), instance().currentFbo->getSize()});
    gl::pushMatrices();
    gl::context()->pushBoolState(GL_DEPTH_TEST, false);
    gl::context()->pushDepthMask(false);
    gl::clear();

    // translate to center and scale to fit whole screen
    gl::translate(getWindowCenter());
    gl::scale(getWindowSize());
}

void PostProcess::End()
{
    gl::context()->popFramebuffer();
    gl::context()->popViewport();
    gl::popMatrices();
    gl::context()->popBoolState(GL_DEPTH_TEST);
    gl::context()->popDepthMask();

    // swap current fbo for write
    instance().swapFbo();
}

const gl::Texture2dRef& PostProcess::getColorTexture() const
{
    // returns the color texture of the fbo before swap
    return currentFbo == auxiliaryFbo ? colorTexture : auxiliaryTexture;
}

void PostProcess::resizeFbos()
{
    // temporal accumulated texture
    gl::Texture2d::Format colorAccFormat = gl::Texture2d::Format().internalFormat(GL_RGBA16F)
                                                                  .magFilter(GL_NEAREST)
                                                                  .minFilter(GL_NEAREST)
                                                                  .wrap(GL_REPEAT);
    const ivec2 winSize = getWindowSize();
    const int32_t h = winSize.y;
    const int32_t w = winSize.x;

    try
    {
        // create temporal texture
        colorTexture = gl::Texture2d::create(w, h, colorAccFormat);
        auxiliaryTexture = gl::Texture2d::create(w, h, colorAccFormat);

        // post process result fbo 
        gl::Fbo::Format colorFormat;
        colorFormat.attachment(GL_COLOR_ATTACHMENT0, colorTexture);
        colorFormat.depthBuffer();
        colorFbo = gl::Fbo::create(w, h, colorFormat);

        // post process auxiliary result fbo
        gl::Fbo::Format auxColorFormat;
        auxColorFormat.attachment(GL_COLOR_ATTACHMENT0, auxiliaryTexture);
        auxColorFormat.depthBuffer();
        auxiliaryFbo = gl::Fbo::create(w, h, auxColorFormat);
    }
    catch (const Exception& e)
    {
        CI_LOG_EXCEPTION("Fbo/Renderbuffer create", e);
    }
}

PostProcess& PostProcess::instance()
{
    static PostProcess postProcess;
    return postProcess;
}

void PostProcess::swapFbo()
{
    currentFbo = currentFbo == auxiliaryFbo ? colorFbo : auxiliaryFbo;
}

PostProcess::PostProcess()
{
    // create fbos
    resizeFbos();
    // post process programs 
    auto toneMappingProg = gl::GlslProg::create(gl::GlslProg::Format()
        .vertex(loadAsset("shaders/fs_quad.vert"))
        .fragment(loadAsset("shaders/tonemapping.frag")));
    auto fxaaProg = gl::GlslProg::create(gl::GlslProg::Format()
        .vertex(loadAsset("shaders/fs_quad.vert"))
        .fragment(loadAsset("shaders/fxaa.frag")));
    auto blurProg = gl::GlslProg::create(gl::GlslProg::Format()
        .vertex(loadAsset("shaders/fs_quad.vert"))
        .fragment(loadAsset("shaders/blur.frag")));
    auto inverseProg = gl::GlslProg::create(gl::GlslProg::Format()
        .vertex(loadAsset("shaders/fs_quad.vert"))
        .fragment(loadAsset("shaders/inverse.frag")));
    auto multiplyProg = gl::GlslProg::create(gl::GlslProg::Format()
        .vertex(loadAsset("shaders/fs_quad.vert"))
        .fragment(loadAsset("shaders/multiply.frag")));
    // create fs quad for single texture display
    const gl::GlslProgRef stockTexture = gl::context()->getStockShader(gl::ShaderDef().texture(GL_TEXTURE_2D));
    const gl::VboMeshRef rect = gl::VboMesh::create(geom::Rect());
    textureRect = gl::Batch::create(rect, stockTexture);
    toneMappingRect = gl::Batch::create(rect, toneMappingProg);
    fxaaRect = gl::Batch::create(rect, fxaaProg);
    blurRect = gl::Batch::create(rect, blurProg);
    inverseRect = gl::Batch::create(rect, inverseProg);
    multiplyRect = gl::Batch::create(rect, multiplyProg);
    // sets targets
    currentFbo = colorFbo;
}
