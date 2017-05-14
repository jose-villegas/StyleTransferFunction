#pragma once
#include <cinder/gl/gl.h>

class PostProcess
{
public:
    /**
     * \brief Displays the given texture on a full screen quad. If colorTex is 
     * null then it displays the color output of the last post-process effect
     * \param colorTex The texture to display
     */
    void displayTexture(const ci::gl::Texture2dRef colorTex = nullptr) const;
    /**
    * \brief Displays the given texture with fast approximate anti-aliasing post effect
    * \param texture The color buffer texture
    */
    void displayFXAA(const ci::gl::Texture2dRef &texture = nullptr) const;
    /**
     * \brief Reinhard tonemapping and gamma correction
     * \param hdrBuffer An unclampled (floating point) color buffer
     * \param local if true will call Start and End at the beginning and the end of function respectively
     */
    void toneMapping(const ci::gl::Texture2dRef& hdrBuffer = nullptr, bool local = true) const;
    /**
     * \brief Horizontal pass to generate a gaussian blurred texture from the given texture
     * \param texture The source texture to blur, if null uses the current color texture
     * \param local if true will call Start and End at the beginning and the end of function respectively
     */
    void blurHorizontal(int blurType = 1, const ci::gl::Texture2dRef& texture = nullptr, bool local = true) const;
    /**
    * \brief Horizontal pass to generate a gaussian blurred texture from the given texture
    * \param texture The source texture to blur, if null uses the current color texture
    * \param local if true will call Start and End at the beginning and the end of function respectively
    */
    void blurVertical(int blurType = 1, const ci::gl::Texture2dRef& texture = nullptr, bool local = true) const;
    /**
     * \brief Generates de inverse of the given texture
     * \param texture The source texture, if null uses the current color texture
     * \param local if true will call Start and End at the beginning and the end of function respectively
     */
    void inverse(const ci::gl::Texture2dRef& texture = nullptr, bool local = true) const;
    /**
     * \brief Multiplies the given textures
     * \param texture0 The right source texture for multiplication
     * \param texture0 The left source texture for multiplication, if null uses the current color texture
     * \param local if true will call Start and End at the beginning and the end of function respectively
     */
    void multiply(const ci::gl::Texture2dRef& texture0, const ci::gl::Texture2dRef& texture1 = nullptr, bool local = true) const;
    /**
     * \brief 4x4 Average gaussian filter
     * \param texture Texture to blur
     * \param local if true will call Start and End at the beginning and the end of function respectively
     */
    void average(const ci::gl::Texture2dRef& texture = nullptr, bool local = true) const;
    /**
     * \brief Generates the ambient occlusion approximation using screen space ambient occlusion
     * \param position Source positions texture
     * \param normal Source normals texture
     * \param camera Current rendering camera
     * \param local if true will call Start and End at the beginning and the end of function respectively 
     */
    void SSAO(const ci::gl::Texture2dRef& position, const ci::gl::Texture2dRef& normal, const ci::Camera &camera, bool local = true);
    /**
     * \brief Sets the appropiate flags for fullscreen effects and binds the internal Fbo for drawing
     */
    static void Start();
    /**
     * \brief Restores the previous rendering flags and unbinds the internal Fbo
     */
    static void End();
    /**
     * \brief The temporal color texture contains the output color result of
     * the last post-process effect executed
     * \return The temporal color texture
     */
    const ci::gl::Texture2dRef &getColorTexture() const;
    /**
     * \brief Resizes the created framebuffers to the actual window's size
     */
    void resizeFbos();
    /**
     * \brief PostProcess follows a singleton pattern
     * \return The unique PostProcess instance
     */
    static PostProcess& instance();
private:
    // fs screen effects
    ci::gl::BatchRef textureRect;
    ci::gl::BatchRef toneMappingRect;
    ci::gl::BatchRef fxaaRect;
    ci::gl::BatchRef blurRect;
    ci::gl::BatchRef inverseRect;
    ci::gl::BatchRef multiplyRect;
    ci::gl::BatchRef ssaoRect;
    ci::gl::BatchRef averageRect;

    // current color texture
    ci::gl::Texture2dRef colorTexture;
    ci::gl::FboRef colorFbo;
    // auxiliary for when trying to write to itself
    ci::gl::Texture2dRef auxiliaryTexture;
    ci::gl::FboRef auxiliaryFbo;

    // draw to this fbo
    ci::gl::FboRef currentFbo;

    // SSAO
    std::vector<glm::vec3> ssaoKernel;
    ci::gl::Texture2dRef ssaoNoiseTexture;

    void swapFbo();
    PostProcess();
};

