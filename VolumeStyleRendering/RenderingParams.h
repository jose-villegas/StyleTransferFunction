#pragma once
class RenderingParams
{
public:
    static float GetExposure();
    static void SetExposure(const float exposure);
    static float GetGamma();
    static void SetGamma(const float gamma);
    static void FXAAEnabled(const bool enabled);
    static bool FXAAEnabled();
    static void DiffuseShadingEnabled(const bool enabled);
    static bool DiffuseShadingEnabled();
    static void ShadowsEnabled(const bool enabled);
    static bool ShadowsEnabled();
    static void SSAOEnabled(const bool enabled);
    static bool SSAOEnabled();
    static void SSAOBias(const float bias);
    static float SSAOBias();
    static void SSAORadius(const float radius);
    static float SSAORadius();
    static void SSAOPower(const float power);
    static float SSAOPower();
private:
    static float gammaValue;
    static float exposureValue;
    static bool fxaa;
    static bool diffuseShading;
    static bool shadows;
    static bool ssao;
    static float ssaoBias;
    static float ssaoRadius;
    static float ssaoPower;
};

