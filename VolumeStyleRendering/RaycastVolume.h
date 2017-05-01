#pragma once
#include "cinder/gl/gl.h"
class TransferFunction;

class RaycastVolume
{
public:
    void loadFromFile(const glm::vec3& dimensions, const glm::vec3& ratios, const std::string filepath, bool is16Bits = false);
    void drawVolume() const;
    explicit RaycastVolume();
    ~RaycastVolume();
    // getters and setters
    glm::vec3 centerPoint() const;
    const float &getStepScale() const;
    void setStepScale(const float& value);
    const glm::vec3 &getAspectRatios() const;
    void setAspectratios(const glm::vec3& value);
    const std::array<float, 256> &getHistogram() const;
    void setTransferFunction(const std::shared_ptr<TransferFunction> &transferFunction);
private:
    // histogram data
    std::array<float, 256> histogram;

    // cube for positions
    ci::gl::BufferObjRef verticesBuffer;
    ci::gl::BufferObjRef indicesBuffer;
    ci::gl::VaoRef vertexArrayObject;

    // volume texture
    ci::gl::Texture3dRef gradientTexture;
    ci::gl::Texture3dRef volumeTexture;

    // front cube fbo
    ci::gl::FboRef frontFbo;
    ci::gl::RenderbufferRef frontDepth;
    // back cube fbo
    ci::gl::FboRef backFbo;
    ci::gl::RenderbufferRef backDepth;

    // draw positions shader
    cinder::gl::GlslProgRef positionsShader;

    // volume raycast
    cinder::gl::GlslProgRef raycastShader;
    std::shared_ptr<TransferFunction> transferFunction;

    // compute shaders
    cinder::gl::SsboRef histogramSsbo;
    cinder::gl::GlslProgRef histogramCompute;
    cinder::gl::GlslProgRef gradientsCompute;

    // raycast parameters
    glm::vec3 dimensions;
    glm::vec3 stepSize;
    glm::vec3 aspectRatios;
    glm::vec3 scaleFactor;
    float stepScale;
    float maxSize;

    bool isDrawable;

    void drawFrontCubeFace() const;
    void drawBackCubeFace() const;
    void createCubeVbo();
    void createFbos();
    void readVolumeFromFile8(const std::string filepath);
    void readVolumeFromFile16(const std::string filepath);

    void extractHistogram();
    void generateGradients();
};
