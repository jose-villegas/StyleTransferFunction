#pragma once
#include "cinder/gl/gl.h"

class Volume3D
{
public:
    void createFromFile(const glm::vec3& dimensions, const std::string filepath);
    void drawFrontCubeFace() const;
    explicit Volume3D();
    ~Volume3D();
private:
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

    void createCubeVbo();
    void createVolumeTexture(const glm::vec3& dimensions, const std::string filepath);
    void createFbos();
};
