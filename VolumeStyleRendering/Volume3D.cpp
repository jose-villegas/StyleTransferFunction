#include "Volume3D.h"
#include <thread>
#include <cinder/app/AppBase.h>
#include <cinder/Log.h>
using namespace ci;
using namespace glm;

Volume3D::Volume3D() : aspectRatios(1), scaleFactor(vec3(1)), stepScale(1)
{
    // positions shader
    positionsShader = gl::GlslProg::create(gl::GlslProg::Format()
        .vertex(loadFile("shaders/positions.vert"))
        .fragment(loadFile("shaders/positions.frag")));
    // raycast shader
    raycastShader = gl::GlslProg::create(gl::GlslProg::Format()
        .vertex(loadFile("shaders/raycast.vert"))
        .fragment(loadFile("shaders/raycast.frag")));

    // create clockwise bbox for volume rendering
    createCubeVbo();
    // create frame buffer object
    createFbos();
}

Volume3D::~Volume3D() {}

const vec3 Volume3D::centerPoint() const
{
    return vec3(0.5) * scaleFactor;
}

const float &Volume3D::getStepScale() const
{
    return stepScale;
}

void Volume3D::setStepScale(const float& value)
{
    stepScale = max(value, 0.01f);
}

const vec3 &Volume3D::getAspectRatios() const
{
    return aspectRatios;
}

void Volume3D::setAspectratios(const vec3& value)
{
    aspectRatios = max(value, vec3(0));
    // volume scale
    scaleFactor = vec3(1) / ((vec3(1) * maxSize) / (dimensions * aspectRatios));
}

void Volume3D::createCubeVbo()
{
    GLfloat vertices[24] =
    {
        0.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 1.0f,
        0.0f, 1.0f, 0.0f,
        0.0f, 1.0f, 1.0f,
        1.0f, 0.0f, 0.0f,
        1.0f, 0.0f, 1.0f,
        1.0f, 1.0f, 0.0f,
        1.0f, 1.0f, 1.0f
    };

    GLuint indices[36] =
    {
        1, 5, 7,
        7, 3, 1,
        0, 2, 6,
        6, 4, 0,
        0, 1, 3,
        3, 2, 0,
        7, 5, 4,
        4, 6, 7,
        2, 3, 7,
        7, 6, 2,
        1, 0, 4,
        4, 5, 1
    };

    // setup counter clock wise bbox data
    verticesBuffer = gl::BufferObj::create(GL_ARRAY_BUFFER, 24 * sizeof(GLfloat), vertices, GL_STATIC_DRAW);
    indicesBuffer = gl::BufferObj::create(GL_ELEMENT_ARRAY_BUFFER, 36 * sizeof(GLuint), indices, GL_STATIC_DRAW);
    // setup vao for object
    vertexArrayObject = gl::Vao::create();
    vertexArrayObject->bind();
    // vertex attributes locations
    gl::enableVertexAttribArray(0);
    // bind and link data
    verticesBuffer->bind();
    gl::vertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, static_cast<GLfloat *>(nullptr));
    indicesBuffer->bind();
}

void Volume3D::readVolumeFromFile8(const vec3& dimensions, const std::string filepath)
{
    // open file and read
    std::ifstream file(filepath, std::ios::binary | std::ios::ate);

    // error opening given filepath
    if (!file.good()) return;

    // determine volume size
    std::streamsize size = file.tellg();
    file.seekg(0, std::ios::beg);
    // reserve space for data
    std::vector<uint8_t> buffer(size);

    if (file.read(reinterpret_cast<char*>(buffer.data()), size))
    {
        // create 3D texture
        auto format = gl::Texture3d::Format().magFilter(GL_LINEAR)
                                             .minFilter(GL_LINEAR)
                                             .wrapS(GL_CLAMP_TO_EDGE)
                                             .wrapR(GL_CLAMP_TO_EDGE)
                                             .wrapT(GL_CLAMP_TO_EDGE);
        format.setDataType(GL_UNSIGNED_BYTE);
        format.setInternalFormat(GL_R8);
        volumeTexture = gl::Texture3d::create(buffer.data(), GL_RED, dimensions.x, dimensions.y, dimensions.z, format);
        // finally has drawable data
        isDrawable = true;
    }

    file.close();
}

void Volume3D::readVolumeFromFile16(const vec3& dimensions, const std::string filepath)
{
    // open file and read
    std::ifstream file(filepath, std::ios::binary | std::ios::ate);

    // error opening given filepath
    if (!file.good()) return;

    // determine volume size
    std::streamsize size = file.tellg();
    file.seekg(0, std::ios::beg);
    // reserve space for data
    std::vector<uint16_t> buffer(size);

    if (file.read(reinterpret_cast<char*>(buffer.data()), size))
    {
        // create 3D texture
        auto format = gl::Texture3d::Format().magFilter(GL_LINEAR)
                                             .minFilter(GL_LINEAR)
                                             .wrapS(GL_CLAMP_TO_EDGE)
                                             .wrapR(GL_CLAMP_TO_EDGE)
                                             .wrapT(GL_CLAMP_TO_EDGE);
        format.setDataType(GL_UNSIGNED_SHORT);
        format.setInternalFormat(GL_R16);
        volumeTexture = gl::Texture3d::create(buffer.data(), GL_RED, dimensions.x, dimensions.y, dimensions.z, format);
        // finally has drawable data
        isDrawable = true;
    }

    file.close();
}

void Volume3D::createFbos()
{
    gl::Fbo::Format format = gl::Fbo::Format();
    format.colorTexture(gl::Texture::Format().wrapS(GL_REPEAT)
                                             .wrapT(GL_REPEAT)
                                             .minFilter(GL_NEAREST)
                                             .magFilter(GL_NEAREST)
                                             .internalFormat(GL_RGBA16F)
                                             .dataType(GL_FLOAT));
    format.depthBuffer(GL_DEPTH_COMPONENT16);

    try
    {
        frontDepth = gl::Renderbuffer::create(app::getWindowWidth(), app::getWindowHeight(), GL_DEPTH_COMPONENT16);
        frontFbo = gl::Fbo::create(app::getWindowWidth(), app::getWindowHeight(), format);
        backDepth = gl::Renderbuffer::create(app::getWindowWidth(), app::getWindowHeight(), GL_DEPTH_COMPONENT16);
        backFbo = gl::Fbo::create(app::getWindowWidth(), app::getWindowHeight(), format);
    }
    catch (const Exception& e)
    {
        CI_LOG_EXCEPTION("Fbo/Renderbuffer create", e);
    }

    CI_CHECK_GL();
}

void Volume3D::createFromFile(const vec3& dimensions, const vec3& ratios, const std::string filepath, bool is16Bits)
{
    // create volume texture
    is16Bits ? readVolumeFromFile16(dimensions, filepath) : readVolumeFromFile8(dimensions, filepath);
    // compute step size and number of iterations for the given volume dimensions
    maxSize = max(dimensions.x, max(dimensions.y, dimensions.z));
    stepSize = vec3(1.0f / (dimensions.x * (maxSize / dimensions.x)),
                    1.0f / (dimensions.y * (maxSize / dimensions.y)),
                    1.0f / (dimensions.z * (maxSize / dimensions.z)));
    this->dimensions = dimensions;
    setAspectratios(ratios);
}

void Volume3D::drawFrontCubeFace() const
{
    if (!isDrawable) return;

    gl::enable(GL_CULL_FACE, true);
    gl::cullFace(GL_BACK);
    vertexArrayObject->bind();
    gl::drawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, static_cast<GLuint *>(nullptr));
    gl::disable(GL_CULL_FACE);
}

void Volume3D::drawBackCubeFace() const
{
    if (!isDrawable) return;

    gl::enable(GL_CULL_FACE, true);
    gl::cullFace(GL_FRONT);
    vertexArrayObject->bind();
    gl::drawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, static_cast<GLuint *>(nullptr));
    gl::disable(GL_CULL_FACE);
}

void Volume3D::drawVolume() const
{
    if (!isDrawable) return;

    // draw front face cube positions to render target
    frontFbo->bindFramebuffer(GL_DRAW_FRAMEBUFFER);
    gl::clear();
    positionsShader->bind();
    positionsShader->uniform("scaleFactor", scaleFactor);
    gl::setDefaultShaderVars();
    drawFrontCubeFace();
    frontFbo->unbindFramebuffer();
    // draw back face cube positions to render target
    backFbo->bindFramebuffer(GL_DRAW_FRAMEBUFFER);
    gl::clear();
    drawBackCubeFace();
    backFbo->unbindFramebuffer();
    // raycast volume
    gl::clear();
    raycastShader->bind();
    // scale volume
    gl::setDefaultShaderVars();
    // bind front and back cube textures
    frontFbo->getTexture2d(GL_COLOR_ATTACHMENT0)->bind(0);
    raycastShader->uniform("cubeFront", 0);
    backFbo->getTexture2d(GL_COLOR_ATTACHMENT0)->bind(1);
    raycastShader->uniform("cubeBack", 1);
    // bind volume data texture
    volumeTexture->bind(2);
    raycastShader->uniform("volume", 2);
    // raycast parameters
    raycastShader->uniform("scaleFactor", scaleFactor);
    raycastShader->uniform("stepSize", stepSize * stepScale);
    raycastShader->uniform("iterations", static_cast<int>(maxSize * (1.0f / stepScale) * 2.0f));
    // draw cube
    {
        gl::ScopedBlend blend(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        vertexArrayObject->bind();
        gl::drawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, static_cast<GLuint *>(nullptr));
    }
}
