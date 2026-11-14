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
std::shared_ptr<StyleTransferFunctionUi> VolumeRenderingAppUi::transferFunctionUi;
// light from the upper left, in front of the initial camera
vec3 VolumeRenderingAppUi::lightRotation = vec3(-42.0f, 40.0f, 0.0f);
vec3 VolumeRenderingAppUi::lightAmbient = vec3(0.1f);
vec3 VolumeRenderingAppUi::lightDiffuse = vec3(1.0f);
float VolumeRenderingAppUi::lightIntensity = 1.0f;

void VolumeRenderingAppUi::Initialize(RaycastVolume& volume)
{
    transferFunctionUi = std::make_shared<StyleTransferFunctionUi>();
    volume.setTransferFunction(transferFunctionUi->getTranferFunction());
    ApplyLight(volume);
}

void VolumeRenderingAppUi::ApplyLight(RaycastVolume& volume)
{
    vec3 rads = radians(lightRotation);
    vec3 direction = vec3(0, 0, 1) * mat3(eulerAngleXYZ(rads.x, rads.y, rads.z));
    volume.setLight(direction, lightAmbient, lightDiffuse * lightIntensity);
}

void VolumeRenderingAppUi::SetLight(RaycastVolume& volume, const vec3& rotation, float intensity)
{
    lightRotation = rotation;
    lightIntensity = intensity;
    ApplyLight(volume);
}

void VolumeRenderingAppUi::ShowWindows(bool rendering, bool lighting, bool transferFunction)
{
    showVolumeOptions = true;
    showRendering = rendering;
    showLightingSetup = lighting;
    showTransferFunction = transferFunction;
}

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

    transferFunctionUi->drawUi(showTransferFunction, volume);
}

void VolumeRenderingAppUi::DrawRenderingOptions(RaycastVolume& volume)
{
    static bool showFps = true;

    if (showFps)
    {
        static ImVec2 rectSize;
        ui::SetNextWindowPos(ImVec2(ci::app::getWindowWidth() - rectSize.x * 1.55, ci::app::getWindowHeight() - rectSize.y * 2.25));
        ui::Begin("##FPS", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings);
        ui::Text("Framerate: (%.1f)", ui::GetIO().Framerate);
        rectSize = ui::GetItemRectSize();
        ui::End();
    }

    if (showRendering)
    {
        ui::SetNextWindowPos(ImVec2(20, ci::app::getWindowHeight() - 330.0f), ImGuiSetCond_FirstUseEver);

        if (!ui::Begin("Rendering", &showRendering, ImGuiWindowFlags_AlwaysAutoResize))
        {
            ui::End();
            return;
        }
        
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

        ui::SliderFloat3("Crop Min", value_ptr(volume.cropMin), 0.0f, 1.0f);
        ui::SliderFloat3("Crop Max", value_ptr(volume.cropMax), 0.0f, 1.0f);
        ui::SliderFloat("Style Lookahead", &volume.styleLookahead, 0.0f, 4.0f);

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

        if (ui::TreeNode("SSAO (classic lighting)"))
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
}

void VolumeRenderingAppUi::DrawLightingSetup(RaycastVolume& volume)
{
    if (showLightingSetup)
    {
        ui::SetNextWindowPos(ImVec2(ci::app::getWindowWidth() - 470.0f, 40), ImGuiSetCond_FirstUseEver);

        if(!ui::Begin("Lighting", &showLightingSetup, ImGuiWindowFlags_AlwaysAutoResize))
        {
            ui::End();
            return;
        }

        auto& volumetric = RenderingParams::Volumetric();
        int model = volumetric.enabled ? 1 : 0;

        ui::Text("Model:");
        ui::SameLine();
        ui::RadioButton("Classic", &model, 0);
        ui::SameLine();
        ui::RadioButton("Volumetric", &model, 1);
        volumetric.enabled = model == 1;

        bool changed = false;
        changed |= ui::SliderFloat3("Rotation", value_ptr(lightRotation), -180, 180);
        changed |= ui::DragFloat3("Ambient", value_ptr(lightAmbient), 0.01f, 0, 1);
        changed |= ui::DragFloat3("Diffuse", value_ptr(lightDiffuse), 0.01f, 0, 1);
        changed |= ui::DragFloat("Intensity", &lightIntensity, 0.1f, 0, 32);

        if (changed) { ApplyLight(volume); }

        ui::Separator();

        if (volumetric.enabled)
        {
            ui::SliderFloat("Gradient Shading", &volumetric.gradientShading, 0.0f, 1.0f);
            ui::SliderFloat("Ambient Strength", &volumetric.ambientStrength, 0.0f, 8.0f);
            ui::SliderFloat("Specular", &volumetric.specularStrength, 0.0f, 2.0f);
            ui::SliderFloat("Shininess", &volumetric.shininess, 1.0f, 128.0f);
            ui::Checkbox("Volumetric Shadows", &volumetric.shadows);
            ui::Checkbox("Ambient Occlusion", &volumetric.occlusion);
            ui::SliderFloat("Occlusion Radius", &volumetric.occlusionRadius, 2.0f, 32.0f);
            ui::Checkbox("Indirect Light", &volumetric.indirect);
            ui::SliderFloat("Indirect Strength", &volumetric.indirectStrength, 0.0f, 4.0f);
            ui::Text("Lighting volumes:");
            ui::SameLine();
            ui::RadioButton("Full", &volumetric.volumeDivisor, 1);
            ui::SameLine();
            ui::RadioButton("1/2", &volumetric.volumeDivisor, 2);
            ui::SameLine();
            ui::RadioButton("1/4", &volumetric.volumeDivisor, 4);
            ui::Text("Last rebuild: %.1f ms", volume.getLightingUpdateTime());
        }
        else
        {
            bool doShading = RenderingParams::DiffuseShadingEnabled();
            bool shadows = RenderingParams::ShadowsEnabled();

            if (ui::Checkbox("Enable Diffuse Shading", &doShading))
            {
                RenderingParams::DiffuseShadingEnabled(doShading);
            }

            if (ui::Checkbox("Enable Shadows", &shadows))
            {
                RenderingParams::ShadowsEnabled(shadows);
            }
        }

        ui::End();
    }
}

void VolumeRenderingAppUi::DrawRotationControls(RaycastVolume& volume)
{
    if (showRotationControls)
    {
        if(!ui::Begin("Rotate", &showRotationControls, ImGuiWindowFlags_AlwaysAutoResize))
        {
            ui::End();
            return;
        }

        static vec3 angles = vec3(0);
        bool changed = false;
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