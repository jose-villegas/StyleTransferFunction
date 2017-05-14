#include <cinder/CinderGlm.h>

#include "RenderingParams.h"

using namespace glm;

float RenderingParams::gammaValue = 2.2f;
float RenderingParams::exposureValue = 1.0f;
bool RenderingParams::fxaa = true;
bool RenderingParams::diffuseShading = true;
bool RenderingParams::shadows = true;
bool RenderingParams::ssao = true;
float RenderingParams::ssaoBias = 0.025f;
float RenderingParams::ssaoRadius = 0.5f;
float RenderingParams::ssaoPower = 1.0f;

float RenderingParams::GetExposure() 
{
    return exposureValue;
}

void RenderingParams::SetExposure(const float exposure)
{
    exposureValue = min(max(epsilon<float>(), exposure), 8.0f);
}

float RenderingParams::GetGamma()
{
    return gammaValue;
}

void RenderingParams::SetGamma(const float gamma)
{
    gammaValue = min(max(epsilon<float>(), gamma), 10.0f);
}

void RenderingParams::FXAAEnabled(const bool enabled)
{
    fxaa = enabled;
}

bool RenderingParams::FXAAEnabled()
{
    return fxaa;
}

void RenderingParams::DiffuseShadingEnabled(const bool enabled)
{
    diffuseShading = enabled;
}

bool RenderingParams::DiffuseShadingEnabled()
{
    return diffuseShading;
}

void RenderingParams::ShadowsEnabled(const bool enabled)
{
    shadows = enabled;
}

bool RenderingParams::ShadowsEnabled()
{
    return shadows;
}

void RenderingParams::SSAOEnabled(const bool enabled)
{
    ssao = enabled;
}

bool RenderingParams::SSAOEnabled()
{
    return ssao;
}

void RenderingParams::SSAOBias(const float bias)
{
    ssaoBias = clamp(bias, 0.0f, 0.5f);
}

float RenderingParams::SSAOBias()
{
    return ssaoBias;
}

void RenderingParams::SSAORadius(const float radius)
{
    ssaoRadius = clamp(radius, 0.001f, 8.0f);
}

float RenderingParams::SSAORadius()
{
    return ssaoRadius;
}

void RenderingParams::SSAOPower(const float power)
{
    ssaoPower = clamp(power, 0.0f, 32.0f);
}

float RenderingParams::SSAOPower()
{
    return ssaoPower;
}
