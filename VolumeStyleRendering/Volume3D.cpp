#include "Volume3D.h"
#include <thread>
#include <cinder/app/AppBase.h>
#include <cinder/Log.h>
using namespace ci;
using namespace glm;

Volume3D::Volume3D() {}

Volume3D::~Volume3D() {}

void Volume3D::createCubeVbo()
{
    // object already created
    if (verticesBuffer || indicesBuffer || vertexArrayObject) return;

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
    gl::enableVertexAttribArray(1);
    // bind and link data
    verticesBuffer->bind();
    gl::vertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, static_cast<GLfloat *>(nullptr));
    gl::vertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, static_cast<GLfloat *>(nullptr));
    indicesBuffer->bind();
}

void Volume3D::createVolumeTexture(const vec3& dimensions, const std::string filepath)
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
    std::vector<float> scalars(size);

    if (file.read(reinterpret_cast<char*>(buffer.data()), size))
    {
        //// multi threaded volume read
        //unsigned nThreads = std::thread::hardware_concurrency() * 2;
        //unsigned segmentSize = ceil(static_cast<float>(buffer.size()) / nThreads);
        //std::vector<std::thread> workThreads(nThreads);

        //for (unsigned i = 0; i < nThreads; i++)
        //{
        //    workThreads[i] = std::thread
        //    ([&scalars, &buffer](unsigned start, unsigned end)
        //     {
        //         for (unsigned j = start; j < end && j < scalars.size(); j++)
        //         {
        //             scalars[j] = static_cast<float>(buffer[j]) / std::numeric_limits<uint8_t>::max();
        //         }
        //     }, i * segmentSize, i * segmentSize + segmentSize);
        //}

        //std::for_each(workThreads.begin(), workThreads.end(), [&workThreads](std::thread& thread) { thread.join(); });

        // create 3D texture
        auto format = gl::Texture3d::Format().magFilter(GL_LINEAR)
                                             .minFilter(GL_LINEAR)
                                             .wrapS(GL_CLAMP_TO_EDGE)
                                             .wrapR(GL_CLAMP_TO_EDGE)
                                             .wrapT(GL_CLAMP_TO_EDGE);
        format.setDataType(GL_UNSIGNED_BYTE);
        format.setInternalFormat(GL_R8);
        volumeTexture = gl::Texture3d::create(scalars.data(), GL_RED, dimensions.x, dimensions.y, dimensions.z, format);
    }

    file.close();
}

void Volume3D::createFbos()
{
    // render targets already created
    if (frontDepth || backDepth) return;

    gl::Fbo::Format format = gl::Fbo::Format();
    format.colorTexture(gl::Texture::Format().wrapS(GL_REPEAT)
                                             .wrapT(GL_REPEAT)
                                             .minFilter(GL_NEAREST)
                                             .magFilter(GL_NEAREST)
                                             .internalFormat(GL_RGBA32F)
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

void Volume3D::createFromFile(const vec3& dimensions, const std::string filepath)
{
    // create clockwise bbox for volume rendering
    createCubeVbo();
    // create volume texture
    createVolumeTexture(dimensions, filepath);
    // create frame buffer object
    createFbos();
}
