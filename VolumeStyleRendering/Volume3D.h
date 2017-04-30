#pragma once
#include "cinder/gl/gl.h"

class Volume3D
{
public:
    void createFromFile(const glm::vec3& dimensions, const glm::vec3& ratios, const std::string filepath, bool is16Bits = false);
    void drawFrontCubeFace() const;
    void drawBackCubeFace() const;
    void drawVolume() const;
    explicit Volume3D();
    ~Volume3D();
    // getters and setters
    const glm::vec3 centerPoint() const;
    const float &getStepScale() const;
    void setStepScale(const float& value);
    const glm::vec3 &getAspectRatios() const;
    void setAspectratios(const glm::vec3& value);
    const std::array<float, 256> &getHistogram() const;
private:
    // histogram data
    std::array<float, 256> histogram;
    // cube for positions
    ci::gl::BufferObjRef verticesBuffer;
    ci::gl::BufferObjRef indicesBuffer;
    ci::gl::VaoRef vertexArrayObject;
    // volume texture
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
    // compute shaders
    cinder::gl::SsboRef histogramSsbo;
    cinder::gl::GlslProgRef histogramCompute;
    // raycast parameters
    glm::vec3 dimensions;
    glm::vec3 stepSize;
    glm::vec3 aspectRatios;
    glm::vec3 scaleFactor;
    float stepScale;
    float maxSize;

    bool isDrawable;

    void createCubeVbo();
    void createFbos();
    void readVolumeFromFile8(const glm::vec3& dimensions, const std::string filepath);
    void readVolumeFromFile16(const glm::vec3& dimensions, const std::string filepath);

    template<typename T, typename = typename std::enable_if<std::is_arithmetic<T>::value, T>::type>
    void extractHistogram(std::vector<T> volume);
};
