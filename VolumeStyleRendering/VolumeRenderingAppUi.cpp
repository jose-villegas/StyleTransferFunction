#include <CinderImGui.h>

#include "VolumeRenderingAppUi.h"
#include "RaycastVolume.h"
#include "StyleTransferFunctionUi.h"
#include "RenderingParams.h"

using namespace glm;

bool VolumeRenderingAppUi::loadNewVolume = false;
bool VolumeRenderingAppUi::showVolumeOptions = false;
bool VolumeRenderingAppUi::showRendering = false;
bool VolumeRenderingAppUi::showTransferFunction = false;
bool VolumeRenderingAppUi::showRotationControls = false;
bool VolumeRenderingAppUi::showLightingSetup = false;
std::string VolumeRenderingAppUi::path;

void VolumeRenderingAppUi::DrawUi(RaycastVolume& volume)
{
    // menu bar
    DrawMainMenuBar(volume);
    // rotation controls
    DrawRotationControls(volume);
    // model lighting setup
    DrawLightingSetup(volume);
    // volume controls
    DrawRenderingOptions(volume);

    // open modal dialog for volume parameters
    if (loadNewVolume)
    {
        ui::OpenPopup("Volume Parameters");
        bool isClosed = VolumeLoadPopup(volume);
        loadNewVolume = !isClosed;
        showVolumeOptions = isClosed;
    }

    // initialize transfer function ui
    static std::shared_ptr<StyleTransferFunctionUi> transferFunctionUi = nullptr;

    if(!transferFunctionUi)
    {
        transferFunctionUi = std::make_shared<StyleTransferFunctionUi>();
        volume.setTransferFunction(transferFunctionUi->getTranferFunction());
    }
    else
    {
        transferFunctionUi->drawUi(showTransferFunction, volume);
    }
}

void VolumeRenderingAppUi::DrawRenderingOptions(RaycastVolume& volume)
{
    static bool showFps = true;

    if (showRendering)
    {
        ui::Begin("Rendering", &showRendering, ImGuiWindowFlags_AlwaysAutoResize);
        static float stepScale = volume.getStepScale();
        static float sStepScale = volume.getShadowStepScale();
        static vec3 aspectRatios = volume.getAspectRatios();
        static int renderingPath = 0;
        static float exposure = RenderingParams::GetExposure();
        static float gamma = RenderingParams::GetGamma();

        stepScale = volume.getStepScale();
        aspectRatios = volume.getAspectRatios();

        if (ui::SliderFloat("Step Scale", &stepScale, 0.1, 8))
        {
            volume.setStepScale(stepScale);
        }

        if (ui::SliderFloat("Shadow Step Scale", &sStepScale, 0.2, 16))
        {
            volume.setShadowStepScale(sStepScale);
        }

        if (ui::InputFloat3("Aspect", value_ptr(aspectRatios)))
        {
            volume.setAspectratios(aspectRatios);
        }

        if(ui::SliderFloat("Gamma", &gamma, 0.01f, 10.0f))
        {
            RenderingParams::SetGamma(gamma);
        }

        if (ui::SliderFloat("Exposure", &exposure, 0.01f, 8.0f))
        {
            RenderingParams::SetExposure(exposure);
        }
        
        ui::Checkbox("Show FPS", &showFps);

        ui::Separator();
        ui::Text("Post-effects");

        if(ui::TreeNode("FXAA"))
        {
            static bool fxaa = RenderingParams::FXAAEnabled();

            if (ui::Checkbox("Enable", &fxaa))
            {
                RenderingParams::FXAAEnabled(fxaa);
            }

            ui::TreePop();
        }

        if (ui::TreeNode("SSAO"))
        {
            static bool ssao = RenderingParams::SSAOEnabled();
            static float ssaoRadius = RenderingParams::SSAORadius();;
            static float ssaoBias = RenderingParams::SSAOBias();;
            static float ssaoPower = RenderingParams::SSAOPower();;

            if (ui::Checkbox("Enable", &ssao))
            {
                RenderingParams::SSAOEnabled(ssao);
            }

            if(ui::SliderFloat("Radius", &ssaoRadius, 0.001f, 8.0f))
            {
                RenderingParams::SSAORadius(ssaoRadius);
            }

            if (ui::SliderFloat("Bias", &ssaoBias, 0.0f, 0.5f))
            {
                RenderingParams::SSAOBias(ssaoBias);
            }

            if (ui::SliderFloat("Power", &ssaoPower, 0.0f, 32.0f))
            {
                RenderingParams::SSAOPower(ssaoPower);
            }

            ui::TreePop();
        }

        ui::End();
    }

    if (showFps)
    {
        static ImVec2 rectSize;
        ui::SetNextWindowPos(ImVec2(ci::app::getWindowWidth() - rectSize.x * 1.55, ci::app::getWindowHeight() - rectSize.y * 2.25));
        ui::Begin("##FPS", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings);
        ui::Text("Framerate: (%.1f)", ui::GetIO().Framerate);
        rectSize = ui::GetItemRectSize();
        ui::End();
    }
}

void VolumeRenderingAppUi::DrawLightingSetup(RaycastVolume& volume)
{
    if (showLightingSetup)
    {
        static Light light = volume.getLight();
        static float lightIntensity = 1.0f;
        static vec3 rotation;
        static bool doShading = true;
        static bool shadows = true;;
        bool changed = false;
        ui::Begin("Lighting", &showLightingSetup, ImGuiWindowFlags_AlwaysAutoResize);

        if (ui::Checkbox("Enable Diffuse Shading", &doShading))
        {
            RenderingParams::DiffuseShadingEnabled(doShading);
        }

        if (ui::Checkbox("Enable Shadows", &shadows))
        {
            RenderingParams::ShadowsEnabled(shadows);
        }

        changed |= ui::SliderFloat3("Rotation", value_ptr(rotation), -180, 180);
        changed |= ui::DragFloat3("Ambient", value_ptr(light.ambient), 0.01, 0, 1);
        changed |= ui::DragFloat3("Diffuse", value_ptr(light.diffuse), 0.01, 0, 1);
        changed |= ui::DragFloat("Intensity", &lightIntensity, 0.1, 0, 32);

        if (changed)
        {
            vec3 rads = radians(rotation);
            light.direction = vec3(0, 0, 1) * mat3(eulerAngleXYZ(rads.x, rads.y, rads.z));
            volume.setLight(light.direction, light.ambient, light.diffuse * lightIntensity);
        }

        ui::End();
    }
}

void VolumeRenderingAppUi::DrawRotationControls(RaycastVolume& volume)
{
    if (showRotationControls)
    {
        static vec3 angles = vec3(0);
        bool changed = false;
        ui::Begin("Rotate", &showRotationControls, ImGuiWindowFlags_AlwaysAutoResize);
        changed |= ui::SliderAngle("X", &angles.x);
        changed |= ui::SliderAngle("Y", &angles.y);
        changed |= ui::SliderAngle("Z", &angles.z);

        if (changed)
        {
            volume.setRotation(toQuat(eulerAngleXYZ(angles.x, angles.y, angles.z)));
        }

        ui::End();
    }
}

bool VolumeRenderingAppUi::VolumeLoadPopup(RaycastVolume& volume)
{
    bool isClosed = false;

    if (ui::BeginPopupModal("Volume Parameters", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
    {
        static ivec3 slices = ivec3(1);
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
        // slices has to be positive
        slices = max(slices, ivec3(1));

        if (ui::Button("Load", ImVec2(ui::GetContentRegionAvailWidth(), 0)))
        {
            volume.loadFromFile(slices, ratios, path, bits == 1);
            ui::CloseCurrentPopup();
            isClosed = true;
        }

        ui::EndPopup();
    }

    return isClosed;
}

void VolumeRenderingAppUi::DrawMainMenuBar(RaycastVolume& volume)
{
    static bool showFps = true;

    // menu bar on top
    {
        ui::ScopedMainMenuBar menuBar;

        if (ui::BeginMenu("File"))
        {
            if (ui::MenuItem("Open"))
            {
                auto fspath = cinder::app::getOpenFilePath(path, { "raw" });

                if (!fspath.empty())
                {
                    path = fspath.string();
                    loadNewVolume = true;
                }
            }
            ui::EndMenu();
        }

        if (ui::BeginMenu("Volume", showVolumeOptions))
        {
            ui::MenuItem("Rotate", nullptr, &showRotationControls);
            ui::MenuItem("Rendering", nullptr, &showRendering);
            ui::MenuItem("Lighting", nullptr, &showLightingSetup);
            ui::MenuItem("Transfer Function", nullptr, &showTransferFunction);

            ui::EndMenu();
        }
    }
}