#include "cinder/app/App.h"
#include "cinder/app/RendererGl.h"
#include "cinder/gl/gl.h"

#include "CinderImGui.h"
#include "Volume3D.h"

using namespace ci;
using namespace app;

class VolumeRenderingApp : public App
{
public:
    static void prepareSettings(Settings* settings);
    void setup() override;
    void update() override;
    void draw() override;
    void mouseDown(MouseEvent event) override;
private:
    Volume3D volume;
    CameraPersp camera;
};

void VolumeRenderingApp::prepareSettings(Settings* settings)
{
    settings->setWindowSize(800, 600);
}

void VolumeRenderingApp::setup()
{
    auto options = ImGui::Options();
    options.font("fonts/DroidSans.ttf", 16);
    ui::initialize(options);

    // camera initial position
    camera.lookAt(vec3(2, 1, 3), vec3(0));
}

void VolumeRenderingApp::update()
{
    static ivec3 slices = ivec3(1);
    static bool loadNewVolume = false;
    static fs::path path;
    static bool showRendering = false;

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
        // volume ratios
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
            ui::CloseCurrentPopup();
        }

        ui::EndPopup();
    }
    // volume controls
    if(showRendering){
        if (!ui::Begin("Rendering", &showRendering))
        {
            ui::End();
        }

        ui::InputFloat("Step Scale", &volume.stepScale);
        ui::InputFloat3("Aspect", value_ptr(volume.ratios));
        ui::End();
    }

    // refresh flags
    loadNewVolume = false;
    // slices has to be positive
    slices = max(slices, ivec3(1));
}

void VolumeRenderingApp::draw()
{
    gl::clear();
    gl::setMatrices(camera);
    // draw volumetric data
    volume.drawVolume();
}

void VolumeRenderingApp::mouseDown(MouseEvent event) {}

CINDER_APP (VolumeRenderingApp, RendererGl, &VolumeRenderingApp::prepareSettings)
