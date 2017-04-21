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
}

void VolumeRenderingApp::update()
{
    static bool loadNewVolume = false;
    static ivec3 slices = ivec3(1);
    static fs::path path;
    static int bits = 0;
    // menu bar on top
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

    // ui::ShowTestWindow();

    // open modal dialog for volume parameters
    if (loadNewVolume)
    {
        ui::OpenPopup("Volume Parameters");
    }

    if (ui::BeginPopupModal("Volume Parameters", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
    {
        // volume dimensions
        ui::InputInt3("Slices", value_ptr(slices));
        // node bit size
        ui::RadioButton("8 bits", &bits, 0);
        ui::SameLine();
        ui::Dummy(ImVec2(ui::GetContentRegionAvailWidth() / 3, 0));
        ui::SameLine();
        ui::RadioButton("16 bits", &bits, 1);

        if (ui::Button("Load", ImVec2(ui::GetContentRegionAvailWidth(), 0)))
        {
            volume.createFromFile(slices, path.string());
            ui::CloseCurrentPopup();
        }

        ui::EndPopup();
    }

    // refresh flags
    loadNewVolume = false;
    // slices has to be positive
    slices = max(slices, ivec3(1));
}

void VolumeRenderingApp::draw()
{
    gl::clear();
}

void VolumeRenderingApp::mouseDown(MouseEvent event) {}

CINDER_APP (VolumeRenderingApp, RendererGl, &VolumeRenderingApp::prepareSettings)
