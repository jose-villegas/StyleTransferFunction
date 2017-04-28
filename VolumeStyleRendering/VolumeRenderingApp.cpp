#include "cinder/app/App.h"
#include "cinder/app/RendererGl.h"
#include "cinder/gl/gl.h"

#include "CinderImGui.h"
#include "Volume3D.h"
#include <cinder/Log.h>
#include "TransferFunction.h"

using namespace ci;
using namespace app;

class VolumeRenderingApp : public App
{
public:
    static void prepareSettings(Settings* settings);
    void setup() override;
    void drawUi();
    void update() override;
    void draw() override;
    void mouseWheel(MouseEvent event) override;
    void mouseDrag(MouseEvent event) override;
    void mouseDown(MouseEvent event) override;
private:
    vec2 dragStart;
    Volume3D volume;
    TransferFunction transferFunction;
    CameraPersp camera;
    CameraPersp initialCamera;
    float dragPivotDistance;
};

void VolumeRenderingApp::prepareSettings(Settings* settings)
{
    settings->setResizable(false);
    settings->setWindowSize(1280, 720);
    // logging
    log::makeLogger<log::LoggerFile>("log.txt");
}

void VolumeRenderingApp::setup()
{
    auto options = ImGui::Options();
    options.font("fonts/DroidSans.ttf", 16);
    ui::initialize(options);
}

void VolumeRenderingApp::drawUi()
{
    static ivec3 slices = ivec3(1);
    static bool loadNewVolume = false;
    static fs::path path;
    static bool showRendering = false;
    static bool showTransferFunction;

    // menu bar on top
    {
        ui::ScopedMainMenuBar menuBar;

        if (ui::BeginMenu("File"))
        {
            if (ui::MenuItem("Open"))
            {
                path = getOpenFilePath(path, {"raw"});

                if (!path.empty())
                {
                    loadNewVolume = true;
                }
            }
            ui::EndMenu();
        }

        if (ui::BeginMenu("Volume"))
        {
            ui::MenuItem("Rendering", nullptr, &showRendering);
            ui::MenuItem("Transfer Function", nullptr, &showTransferFunction);
            ui::EndMenu();
        }
    }

    // open modal dialog for volume parameters
    if (loadNewVolume)
    {
        ui::OpenPopup("Volume Parameters");
    }

    if (ui::BeginPopupModal("Volume Parameters", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
    {
        static vec3 ratios = vec3(1);
        static int bits = 0;
        // volume dimensions
        ui::InputInt3("Slices", value_ptr(slices));
        // volume aspectRatios
        ui::InputFloat3("Aspect", value_ptr(ratios));
        // node bit size
        ui::RadioButton("8 bits", &bits, 0);
        ui::SameLine();
        ui::Dummy(ImVec2(ui::GetContentRegionAvailWidth() / 3, 0));
        ui::SameLine();
        ui::RadioButton("16 bits", &bits, 1);

        if (ui::Button("Load", ImVec2(ui::GetContentRegionAvailWidth(), 0)))
        {
            volume.createFromFile(slices, ratios, path.string(), bits == 1);
            transferFunction.setVolume(volume);
            // position camera looking at volume
            camera.setEyePoint(volume.centerPoint() + vec3(0, 0, 2));
            camera.lookAt(volume.centerPoint());
            camera.setPivotDistance(2);

            ui::CloseCurrentPopup();
        }

        ui::EndPopup();
    }
    // volume controls
    if (showRendering)
    {
        static float stepScale = volume.getStepScale();
        static vec3 aspectRatios = volume.getAspectRatios();

        if (!ui::Begin("Rendering", &showRendering))
        {
            ui::End();
        }

        stepScale = volume.getStepScale();
        aspectRatios = volume.getAspectRatios();

        if (ui::InputFloat("Step Scale", &stepScale))
        {
            volume.setStepScale(stepScale);
        }

        if (ui::InputFloat3("Aspect", value_ptr(aspectRatios)))
        {
            volume.setAspectratios(aspectRatios);
            // update camera
            auto eye = volume.centerPoint() + camera.getViewDirection() * camera.getPivotDistance();
            camera.setEyePoint(eye);
            camera.lookAt(eye, volume.centerPoint(), camera.getWorldUp());
            dragStart = vec2(0);
            initialCamera = camera;
        }

        ui::End();
    }
    // transfer function ui
    if (showTransferFunction)
    {
        transferFunction.drawUi(showTransferFunction);
    }

    // refresh flags
    loadNewVolume = false;
    // slices has to be positive
    slices = max(slices, ivec3(1));
}

void VolumeRenderingApp::update()
{
    drawUi();
}

void VolumeRenderingApp::draw()
{
    gl::clear();
    gl::setMatrices(camera);
    // draw volumetric data
    volume.drawVolume();
}

void VolumeRenderingApp::mouseWheel(MouseEvent event)
{
    float increment = event.getWheelIncrement();
    float multiplier = powf(1.2f, increment);
    // move camera
    vec3 translate = camera.getViewDirection() * (camera.getPivotDistance() * (1.0f - multiplier));
    camera.setEyePoint(camera.getEyePoint() + translate);
    camera.setPivotDistance(camera.getPivotDistance() * multiplier);
}

void VolumeRenderingApp::mouseDrag(MouseEvent event)
{
    if (event.isLeftDown())
    {
        vec2 dragCurrent = event.getPos();
        float deltaX = (dragCurrent.x - dragStart.x) / -100.0f;
        float deltaY = (dragCurrent.y - dragStart.y) / 100.0f;
        vec3 viewDirection = normalize(initialCamera.getViewDirection());
        bool invertMotion = (initialCamera.getOrientation() * initialCamera.getWorldUp()).y < 0.0f;

        vec3 rightDirection = normalize(cross(initialCamera.getWorldUp(), viewDirection));

        if (invertMotion)
        {
            deltaX = -deltaX;
            deltaY = -deltaY;
        }

        console() << deltaX << ", " << deltaY << std::endl;

        vec3 rotated = angleAxis(deltaY, rightDirection) * (-initialCamera.getViewDirection() * dragPivotDistance);
        rotated = angleAxis(deltaX, initialCamera.getWorldUp()) * rotated;

        camera.setEyePoint(initialCamera.getEyePoint() + initialCamera.getViewDirection() * dragPivotDistance + rotated);
        camera.setOrientation(angleAxis(deltaX, initialCamera.getWorldUp()) * angleAxis(deltaY, rightDirection) * initialCamera.getOrientation());
    }
}

void VolumeRenderingApp::mouseDown(MouseEvent event)
{
    if (event.isLeftDown())
    {
        dragStart = event.getPos();
        initialCamera = camera;
        dragPivotDistance = camera.getPivotDistance();
    }
}

CINDER_APP (VolumeRenderingApp, RendererGl, &VolumeRenderingApp::prepareSettings)
