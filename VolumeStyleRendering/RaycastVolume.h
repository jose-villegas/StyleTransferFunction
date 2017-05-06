#pragma once
#include "cinder/gl/gl.h"
#include "Light.h"

class TransferFunction;

/**
 * \brief Defines a volume rendered with volume raycasting
 */
class RaycastVolume
{
public:
    /**
     * \brief Loads the raw data from the given filepath into a 3d texture
     * \param dimensions The volume dimensions
     * \param ratios The volume aspect ratios
     * \param filepath The volume's raw data filepath
     * \param is16Bits Determines if the volume is 8 o 16 bits depth
     */
    void loadFromFile(const glm::vec3& dimensions, const glm::vec3& ratios, const std::string filepath, bool is16Bits = false);
    /**
     * \brief Renders the volume using raycasting to the specified output
     * \param camera Main rendering camera
     * \param toRendertargets If true it renders the raycasting result to color, normal and depth render targets
     */

    void drawVolume(const cinder::Camera& camera, bool toRendertargets);
    /**
     * \brief Updates frame buffer objects to the current size of the main window
     */
    void resizeFbos();

    explicit RaycastVolume();
    ~RaycastVolume();

    // getters and setters
    /**
     * \brief The volume resides inside a 0-1 bouding box, this returns the post-scale center point
     * \return The volume center point
     */
    glm::vec3 centerPoint() const;
    /**
     * \brief Raycasting's step scale
     * \return The step scale of the raycasting ray
     */
    const float &getStepScale() const;
    /**
     * \brief Sets the volume raycasting's ray step scale
     * \param value The new step scale
     */
    void setStepScale(const float& value);
    /**
     * \brief Volume's aspect ratios per axis
     * \return Volume's aspect ratios
     */
    const glm::vec3 &getAspectRatios() const;
    /**
     * \brief Sets the volume's aspect ratio
     * \param value The new aspect ratio
     */
    void setAspectratios(const glm::vec3& value);
    /**
     * \brief The volume's histogram contains the normalized [0..1] frequencies of each opacity value
     * \return The volume's data histogram
     */
    const std::array<float, 256> &getHistogram() const;
    /**
     * \brief Sets the color mapping transfer function, used to set the color of each opacity value
     * \param transferFunction Color transfer function
     */
    void setTransferFunction(const std::shared_ptr<TransferFunction>& transferFunction);
    /**
     * \brief Enables or disables diffuse shading
     * \param enable Indicates if diffuse shading is enabled
     */
    void diffuseShading(bool enable);
    /**
     * \brief Sets the volume raycast directional light
     * \param direction Light's direction
     * \param ambient Light's ambient color
     * \param diffuse Light's diffuse color
     */
    void setLight(glm::vec3 direction, glm::vec3 ambient, glm::vec3 diffuse);
    /**
     * \brief Directional light used for simple lighting in the volume raycasting
     * \return The volume raycast's directional light
     */
    const Light &getLight() const;
    /**
     * \brief The volume's model rotation
     * \return Model quaternion rotation
     */
    const glm::quat &getRotation() const;
    /**
     * \brief Sets the volume's model rotation
     * \param rotation The new rotation
     */
    void setRotation(const glm::quat& rotation);
    /**
     * \brief The volume's model position
     * \return 3d position in world space
     */
    const glm::vec3 &getPosition() const;
    /**
     * \brief Sets the volume's model position in world space
     * \param position The new volume position in world space
     */
    void setPosition(const glm::vec3& position);

    /**
     * \brief When rendering is done to render targets this texture contains the color output
     * \return The color output render target
     */
    const ci::gl::Texture2dRef &getColorTexture() const;
    /**
     * \brief When rendering is done to render targets this texture contains the normal output
     * \return The normal output render target 
     */
    const cinder::gl::Texture2dRef &getNormalTexture() const;
    /**
     * \brief When rendering is done to render targets this texture contains the depth output 
     * \return The depth output render target  
     */
    const cinder::gl::Texture2dRef &getDepthTexture() const;
private:
    // histogram data
    std::array<float, 256> histogram;

    // cube for positions
    ci::TriMeshRef cubeMesh;
    ci::gl::VboRef cubeVerticesBuffer;
    ci::gl::VboRef cubeIndicesBuffer;
    ci::gl::VaoRef cubeVao;

    // volume texture
    ci::gl::Texture3dRef gradientTexture;
    ci::gl::Texture3dRef volumeTexture;

    // fbos
    ci::gl::FboRef frontFbo;
    ci::gl::FboRef backFbo;
    ci::gl::FboRef volumeRBuffer;

    // draw positions shader
    ci::gl::GlslProgRef positionsShader;

    // volume raycast
    cinder::gl::GlslProgRef raycastShaderRendertargets;
    cinder::gl::GlslProgRef raycastShaderDirect;
    std::shared_ptr<TransferFunction> transferFunction;

    // lighting
    Light light;

    // compute shaders
    ci::gl::SsboRef histogramSsbo;
    ci::gl::GlslProgRef histogramCompute;
    ci::gl::GlslProgRef gradientsCompute;
    ci::gl::GlslProgRef smoothGradientsCompute;

    // render targets
    ci::gl::Texture2dRef frontTexture;
    ci::gl::Texture2dRef backTexture;
    ci::gl::Texture2dRef volumeNormal;
    ci::gl::Texture2dRef volumeColor;

    // raycast parameters
    ci::gl::Texture2dRef noiseTexture;
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

    /**
     * \brief Draws the bounding cube back and front face to the position Rendertargets
     */
    void drawCubeFaces() const;
    /**
     * \brief Creates the bounding cube vertex buffer object, used for drawing
     */
    void createCubeVbo();
    /**
     * \brief Reads the raw data at filepath and creates a 3d texture
     * \param filepath The 8 bits depth raw file path 
     */
    void readVolumeFromFile8(const std::string filepath);
    /**
     * \brief Reads the raw data at filepath and creates a 3d texture
     * \param filepath The 18 bits depth raw file path
     */
    void readVolumeFromFile16(const std::string filepath);
    /**
     * \brief Uses a compute shader to extract the frequency of each opacity value within the volume
     * and creates a normalized histogram with this data
     */
    void extractHistogram();
    /**
     * \brief Creates a 3d texture containing the gradient at each voxel and smooths the values using
     * a 3x3x3 average filter
     */
    void generateGradients();
};
