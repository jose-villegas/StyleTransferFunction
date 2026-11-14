#include <cstdio>
#include <sstream>
#include <windows.h>
#include <shellapi.h>
#pragma comment(lib, "Shell32.lib")

#include <cinder/app/App.h>
#include <cinder/ImageIo.h>
#include <cinder/gl/gl.h>

#include "Harness.h"
#include "RaycastVolume.h"
#include "RenderingParams.h"
#include "StyleTransferFunction.h"
#include "StyleTransferFunctionUi.h"
#include "VolumeRenderingAppUi.h"

using namespace ci;
using namespace glm;

namespace
{
    LONG WINAPI crashHandler(EXCEPTION_POINTERS* info)
    {
        fprintf(stderr, "crash: exception 0x%08lX at %p\n", info->ExceptionRecord->ExceptionCode,
                info->ExceptionRecord->ExceptionAddress);
        fflush(stderr);
        return EXCEPTION_EXECUTE_HANDLER;
    }

    template <typename T, int N>
    vec<N, T, defaultp> parseVector(const std::string& text)
    {
        vec<N, T, defaultp> result(0);
        std::stringstream stream(text);
        std::string item;

        for (int i = 0; i < N && std::getline(stream, item, ','); i++)
        {
            result[i] = static_cast<T>(std::stod(item));
        }

        return result;
    }

    // finds a file relative to the working directory or any parent of the executable
    std::string findFile(const std::string& relative)
    {
        if (fs::exists(relative)) return relative;

        auto dir = app::getAppPath();

        for (int i = 0; i < 5 && !dir.empty(); i++, dir = dir.parent_path())
        {
            if (fs::exists(dir / relative)) return (dir / relative).string();
        }

        return relative;
    }

    int addStyle(const std::string& name)
    {
        const auto path = app::getAssetPath("images/styles/" + name + ".png").string();
        Style::AddStyle(name, path);
        auto& styles = Style::GetAvailableStyles();

        for (int i = 0; i < styles.size(); i++)
        {
            if (styles[i].getFilepath() == path) return i;
        }

        return 0;
    }
}

Harness& Harness::instance()
{
    static Harness harness;
    return harness;
}

void Harness::parseCommandLine()
{
    int count = 0;
    LPWSTR* wide = CommandLineToArgvW(GetCommandLineW(), &count);
    std::vector<std::string> args;

    for (int i = 0; i < count; i++)
    {
        const int length = WideCharToMultiByte(CP_UTF8, 0, wide[i], -1, nullptr, 0, nullptr, nullptr);
        std::string arg(std::max(length - 1, 0), '\0');
        WideCharToMultiByte(CP_UTF8, 0, wide[i], -1, arg.data(), length, nullptr, nullptr);
        args.push_back(arg);
    }

    LocalFree(wide);
    parse(args);
}

void Harness::parse(const std::vector<std::string>& args)
{
    for (size_t i = 1; i < args.size(); i++)
    {
        const auto& arg = args[i];
        const bool hasValue = i + 1 < args.size();
        const std::string value = hasValue ? args[i + 1] : "";
        bool consumed = true;

        if (arg == "--preset" && hasValue) preset = value;
        else if (arg == "--volume" && hasValue) volumePath = value;
        else if (arg == "--dims" && hasValue) dims = parseVector<int, 3>(value);
        else if (arg == "--aspect" && hasValue) aspect = parseVector<float, 3>(value);
        else if (arg == "--lighting" && hasValue) lighting = value == "classic" ? 0 : 1;
        else if (arg == "--light" && hasValue) { light = parseVector<float, 3>(value); hasLight = true; }
        else if (arg == "--camera" && hasValue) { cameraOrbit = parseVector<float, 3>(value); hasCamera = true; }
        else if (arg == "--size" && hasValue) size = parseVector<int, 2>(value);
        else if (arg == "--frames" && hasValue) frames = std::stoi(value);
        else if (arg == "--warmup" && hasValue) warmup = std::stoi(value);
        else if (arg == "--capture" && hasValue) captureDir = value;
        else if (arg == "--orbit" && hasValue) orbit = std::stof(value);
        else if (arg == "--light-orbit" && hasValue) lightOrbit = std::stof(value);
        else if (arg == "--compare" && hasValue) compare = std::stoi(value);
        else if (arg == "--debug-view" && hasValue) debugView = std::stoi(value);
        else if (arg == "--off" && hasValue)
        {
            auto& v = RenderingParams::Volumetric();
            if (value == "shadows") v.shadows = false;
            else if (value == "occlusion") v.occlusion = false;
            else if (value == "indirect") v.indirect = false;
            else if (value == "specular") v.specularStrength = 0.0f;
            else if (value == "gradient") v.gradientShading = 0.0f;
        }
        else if (arg == "--ambient" && hasValue) RenderingParams::Volumetric().ambientStrength = std::stof(value);
        else if (arg == "--intensity" && hasValue) intensity = std::stof(value);
        else if (arg == "--threshold" && hasValue) threshold = parseVector<int, 2>(value);
        else if (arg == "--rotate" && hasValue) { rotation = parseVector<float, 3>(value); hasRotation = true; }
        else if (arg == "--crop" && hasValue)
        {
            const auto box = value.find(',', value.find(',', value.find(',') + 1) + 1);
            cropMin = parseVector<float, 3>(value.substr(0, box));
            cropMax = parseVector<float, 3>(value.substr(box + 1));
            hasCrop = true;
        }
        else
        {
            consumed = false;

            if (arg == "--16bit") is16Bits = true;
            else if (arg == "--ui")
            {
                openUi = true;
                // show the style points, the heart of a style transfer function
                StyleTransferFunctionUi::PointType = 2;
                StyleTransferFunctionUi::OpenPointList = true;
            }
            else if (arg == "--no-ui") noUi = true;
            else continue;
        }

        active = true;

        if (consumed) i++;
    }

    if (active) SetUnhandledExceptionFilter(crashHandler);
}

void Harness::applyPreset(RaycastVolume& volume)
{
    auto transferFunction = volume.getTransferFunction();

    if (preset.empty() || !transferFunction) return;

    // colors stay white, the lit sphere styles give each material its color and shading
    transferFunction->reset();

    if (preset == "bonsai")
    {
        // green leaves, wood, soil and a glazed ceramic pot
        transferFunction->setThreshold(40, 255);
        transferFunction->setAlpha(0, 0.0f);
        transferFunction->addAlphaPoint(0.0f, 40);
        transferFunction->addAlphaPoint(0.8f, 47);
        transferFunction->addAlphaPoint(1.0f, 55);
        transferFunction->addStylePoint(StylePoint(56, addStyle("foliage")));
        transferFunction->addStylePoint(StylePoint(110, addStyle("wood")));
        // the trunk and the soil share densities, the soil around 135 and its pebbles higher. The ceramic wall peaks around 210
        transferFunction->addStylePoint(StylePoint(150, addStyle("soil")));
        transferFunction->addStylePoint(StylePoint(205, addStyle("ceramic")));
        // the plate it stands on shares the leaves' densities
        volume.cropMin = vec3(0.07f, 0.075f, 0.02f);
        volume.cropMax = vec3(0.92f, 1, 1);
        // the rims of the wood, soil and ceramic pass through the leaves' densities
        volume.styleLookahead = 2.0f;
    }
    else if (preset == "aneurism")
    {
        transferFunction->setThreshold(40, 255);
        transferFunction->setAlpha(0, 0.0f);
        transferFunction->addAlphaPoint(0.0f, 50);
        transferFunction->addAlphaPoint(0.9f, 110);
        transferFunction->addStylePoint(StylePoint(100, addStyle("vessel")));
    }
    else if (preset == "engine")
    {
        // a solid steel block, the dense brass parts show where it is cut
        transferFunction->setThreshold(100, 255);
        transferFunction->setAlpha(0, 0.0f);
        transferFunction->addAlphaPoint(0.0f, 100);
        transferFunction->addAlphaPoint(0.85f, 120);
        transferFunction->addAlphaPoint(0.4f, 138);
        transferFunction->addAlphaPoint(0.0f, 170);
        transferFunction->addAlphaPoint(0.9f, 200);
        transferFunction->addStylePoint(StylePoint(155, addStyle("steel")));
        transferFunction->addStylePoint(StylePoint(205, addStyle("brass")));
        // the face of the brass parts passes through the steel's densities
        volume.styleLookahead = 2.0f;
        volume.setRotation(angleAxis(radians(-90.0f), vec3(1, 0, 0)));
    }
    else if (preset == "skull")
    {
        transferFunction->setThreshold(50, 255);
        transferFunction->setAlpha(0, 0.0f);
        transferFunction->addAlphaPoint(0.0f, 50);
        transferFunction->addAlphaPoint(0.6f, 95);
        transferFunction->addStylePoint(StylePoint(100, addStyle("bone")));
        volume.setRotation(angleAxis(radians(-90.0f), vec3(1, 0, 0)));
    }
    else if (preset == "foot")
    {
        // faint skin over bone
        transferFunction->setThreshold(30, 255);
        transferFunction->setAlpha(0, 0.0f);
        transferFunction->addAlphaPoint(0.0f, 35);
        transferFunction->addAlphaPoint(0.015f, 55);
        transferFunction->addAlphaPoint(0.0f, 75);
        transferFunction->addAlphaPoint(0.0f, 95);
        transferFunction->addAlphaPoint(0.8f, 125);
        transferFunction->addStylePoint(StylePoint(55, addStyle("skin")));
        transferFunction->addStylePoint(StylePoint(120, addStyle("bone")));
        volume.setRotation(angleAxis(radians(90.0f), vec3(0, 1, 0)));
    }
}

void Harness::placeCamera(CameraPersp& camera) const
{
    const vec3 angles = vec3(radians(cameraOrbit.x), radians(cameraOrbit.y), 0);
    const vec3 eye = cameraOrbit.z * vec3(sin(angles.x) * cos(angles.y), sin(angles.y), -cos(angles.x) * cos(angles.y));
    camera.lookAt(eye, vec3(0), vec3(0, 1, 0));
}

void Harness::start(RaycastVolume& volume, CameraPersp& camera)
{
    if (!active) return;

    if (!preset.empty() && volumePath.empty())
    {
        dims = preset == "engine" ? ivec3(256, 256, 128) : ivec3(256);
        volumePath = findFile("volumes/" + preset + "_" + std::to_string(dims.x) + "x" + std::to_string(dims.y) + "x" +
                              std::to_string(dims.z) + "_uint8.raw");
    }

    if (!volumePath.empty())
    {
        volume.loadFromFile(vec3(dims), aspect, volumePath, is16Bits);
        VolumeRenderingAppUi::ShowWindows(openUi, openUi, openUi);
    }

    applyPreset(volume);
    volume.debugView = debugView;

    if (threshold.y > 0 && volume.getTransferFunction()) volume.getTransferFunction()->setThreshold(threshold.x, threshold.y);

    if (hasCrop)
    {
        volume.cropMin = cropMin;
        volume.cropMax = cropMax;
    }

    if (hasRotation)
    {
        const vec3 r = radians(rotation);
        volume.setRotation(toQuat(eulerAngleXYZ(r.x, r.y, r.z)));
    }
    printf("OpenGL %s, %s\nvolume %s %s\n", glGetString(GL_VERSION), glGetString(GL_RENDERER),
           volumePath.c_str(), volume.hasVolume() ? "loaded" : "not loaded");
    fflush(stdout);

    if (lighting >= 0) RenderingParams::Volumetric().enabled = lighting == 1;

    if (hasLight || intensity != 1.0f)
    {
        VolumeRenderingAppUi::SetLight(volume, hasLight ? light : VolumeRenderingAppUi::GetLightRotation(), intensity);
    }

    if (!hasCamera && !preset.empty())
    {
        // framing of each bundled volume
        cameraOrbit = preset == "bonsai"     ? vec3(30, 5, 2.0f)
                      : preset == "skull"    ? vec3(-60, 10, 2.8f)
                      : preset == "foot"     ? vec3(-20, 40, 2.6f)
                      : preset == "aneurism" ? vec3(20, 15, 1.8f)
                                             : vec3(35, 30, 1.9f);
        hasCamera = true;
    }

    if (hasCamera) placeCamera(camera);

    if (!captureDir.empty()) fs::create_directories(captureDir);
}

void Harness::update(RaycastVolume& volume, CameraPersp& camera)
{
    if (!active) return;

    const int animated = max(frame - warmup, 0);

    if (orbit != 0.0f)
    {
        cameraOrbit.x += orbit * (frame >= warmup ? 1.0f : 0.0f);
        placeCamera(camera);
    }

    if (lightOrbit != 0.0f && frame >= warmup)
    {
        auto rotation = VolumeRenderingAppUi::GetLightRotation();
        rotation.y += lightOrbit;
        VolumeRenderingAppUi::SetLight(volume, rotation, intensity);
    }

    if (compare > 0)
    {
        RenderingParams::Volumetric().enabled = (animated / compare) % 2 == 1;
    }
}

void Harness::postDraw()
{
    if (!active) return;

    if (frame == warmup) captureStart = app::getElapsedSeconds();

    for (GLenum error = glGetError(); error != GL_NO_ERROR && frame < 3; error = glGetError())
    {
        printf("frame %d: GL error 0x%04X\n", frame, error);
        fflush(stdout);
    }

    if (frame >= warmup && !captureDir.empty())
    {
        char name[64];
        snprintf(name, sizeof(name), "frame_%04d.png", frame - warmup);
        writeImage(fs::path(captureDir) / name, app::copyWindowSurface());
    }

    frame++;

    if (frames > 0 && frame >= warmup + frames)
    {
        const double elapsed = app::getElapsedSeconds() - captureStart;
        printf("frames %d, %.2f ms per frame (including capture)\n", frames, 1000.0 * elapsed / frames);
        fflush(stdout);
        app::App::get()->quit();
    }
}
