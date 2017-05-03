#pragma once
#include "cinder/gl/gl.h"
#include "Light.h"

class TransferFunction;

class RaycastVolume
{
public:
    void loadFromFile(const glm::vec3& dimensions, const glm::vec3& ratios, const std::string filepath, bool is16Bits = false);
    void drawVolume(bool deferredPath = false);
    void resizeFbos();
    explicit RaycastVolume();
    ~RaycastVolume();
    // getters and setters
    glm::vec3 centerPoint() const;
    const float &getStepScale() const;
    void setStepScale(const float& value);
    const glm::vec3 &getAspectRatios() const;
    void setAspectratios(const glm::vec3& value);
    const std::array<float, 256> &getHistogram() const;
    void setTransferFunction(const std::shared_ptr<TransferFunction>& transferFunction);
    void diffuseShading(bool enable);
    void setLight(glm::vec3 direction, glm::vec3 ambient, glm::vec3 diffuse);
    const Light &getLight() const;
    const glm::quat &getRotation() const;
    void setRotation(const glm::quat& rotation);
    const glm::vec3 &getPosition() const;
    void setPosition(const glm::vec3& position);
private:
    // histogram data
    std::array<float, 256> histogram;

    // cube for positions
    ci::TriMeshRef cubeMesh;
    ci::gl::VboRef cubeVerticesBuffer;
    ci::gl::VboRef cubeIndicesBuffer;
    ci::gl::VaoRef cubeVao;

    // full screen quad
    ci::TriMeshRef fsQuadMesh;
    ci::gl::VboRef fsQuadVerticesBuffer;
    ci::gl::VboRef fsQuadTexcoordBuffer;
    ci::gl::VboRef fsQuadIndicesBuffer;
    ci::gl::VaoRef fsQuadVao;

    // volume texture
    ci::gl::Texture3dRef gradientTexture;
    ci::gl::Texture3dRef volumeTexture;

    // fbos
    ci::gl::FboRef frontFbo;
    ci::gl::FboRef backFbo;
    ci::gl::FboRef gBuffer;

    // draw positions shader
    cinder::gl::GlslProgRef positionsShader;

    // volume raycast
    cinder::gl::GlslProgRef raycastShader;
    cinder::gl::GlslProgRef raycastShaderGBuffer;
    std::shared_ptr<TransferFunction> transferFunction;

    // lighting
    cinder::gl::GlslProgRef lightBuffer;
    Light light;

    // compute shaders
    cinder::gl::SsboRef histogramSsbo;
    cinder::gl::GlslProgRef histogramCompute;
    cinder::gl::GlslProgRef gradientsCompute;
    cinder::gl::GlslProgRef smoothGradientsCompute;

    // raycast parameters
    cinder::gl::Texture2dRef frontTexture;
    cinder::gl::Texture2dRef backTexture;
    cinder::gl::Texture2dRef noiseTexture;
    glm::vec3 dimensions;
    glm::vec3 stepSize;
    glm::vec3 aspectRatios;
    glm::vec3 scaleFactor;
    float stepScale;
    float maxSize;
    bool enableDiffuseShading;

    // model
    bool isDrawable;
    glm::quat modelRotation;
    glm::vec3 modelPosition;

    void drawCubeFaces();
    void createCubeVbo();
    void createFullscreenQuad();
    void readVolumeFromFile8(const std::string filepath);
    void readVolumeFromFile16(const std::string filepath);

    void extractHistogram();
    void generateGradients();
};
