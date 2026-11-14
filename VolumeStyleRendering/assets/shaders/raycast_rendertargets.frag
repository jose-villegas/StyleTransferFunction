#version 420
struct Light
{
    vec3 direction;
    vec3 ambient;
    vec3 diffuse;
};

layout(binding=0) uniform sampler2D cubeFront;
layout(binding=1) uniform sampler2D cubeBack;
layout(binding=2) uniform sampler3D volume;
layout(binding=3) uniform sampler3D gradients;
layout(binding=4) uniform sampler2D bakedNoise;
layout(binding=5) uniform sampler1D colorMappingFunction;
layout(binding=6) uniform sampler1D transferFunction;
layout(binding=7) uniform isampler1D indexFunction;
layout(binding=8) uniform sampler2DArray styleFunction;
layout(binding=9) uniform sampler2D volumeAO;
layout(binding=10) uniform sampler3D lightVolume;
layout(binding=11) uniform sampler3D ambientVolume;

uniform mat4 ciModelView;
uniform mat4 ciModelMatrix;
uniform mat3 ciNormalMatrix;
uniform mat3 ciModelMatrixInverseTranspose;
uniform Light light;

uniform vec2 threshold;
uniform vec3 stepSize;
uniform vec3 shadowStepSize;
uniform int iterations;
uniform float stepScale;

// classic lighting: lambert + ambient, screen space shadows and ssao
uniform bool diffuseShading;
uniform bool raycastShadows;
uniform bool ambientOcclusion;

// volumetric lighting
uniform bool volumetricLighting;
uniform vec3 eyePosition;
// direction toward the light in texture space and one light volume voxel along it
uniform vec3 lightOffset;
uniform float gradientShading;
uniform float ambientStrength;
uniform float specularStrength;
uniform float shininess;
uniform bool volumetricShadows;
uniform bool volumeOcclusion;
uniform bool indirectLight;
uniform float indirectStrength;

in vec4 position;

layout (location=0) out vec4 oColor;
layout (location=1) out vec4 oNormal;
layout (location=2) out vec4 oShadow;
layout (location=3) out vec4 oPosition;

vec2 litsphere(vec3 eye, vec3 normal)
{
    vec3 reflected = reflect(eye, normal);

    float m = 2.0 * sqrt
    (
        pow(reflected.x, 2.0) +
        pow(reflected.y, 2.0) +
        pow(reflected.z + 1.0, 2.0)
    );

    return reflected.xy / m + 0.5;
}

vec4 styleMapping(vec3 eye, vec3 normal, float opacity)
{
    // apply litsphere
    float index = texture(transferFunction, opacity).x;

    // obtain index function index and interop weight
    int index0 = int(floor(index));
    int index1 = index0 + 1;
    float weight = index - index0;

    // style function texture index
    int styleIndex0 = texelFetch(indexFunction, index0, 0).x;
    int styleIndex1 = texelFetch(indexFunction, index1, 0).x;

    // no style just take base color
    if(styleIndex0 < 0 && styleIndex1 < 0) return vec4(1);

    // obtain style color at view normal, -1 means no style to interop
    vec4 style0 = texture(styleFunction, vec3(litsphere(eye, normal), styleIndex0));
    vec4 style1 = texture(styleFunction, vec3(litsphere(eye, normal), styleIndex1));

    // interpolate
    return styleIndex0 < 0 ? style1 : styleIndex1 < 0 ? style0 : mix(style0, style1, weight);
}

float voxelOcclusion(vec3 rayStart, vec3 rayDir)
{
    vec3 step = rayDir * shadowStepSize;
    vec3 pos = rayStart + step;

    for(int i = 0; i < iterations; i++)
    {
        float opacity = texture(volume, pos).x;

        if(opacity >= threshold.x && opacity <= threshold.y)
        {
            // assigned color from transfer function for this density
            vec4 src = texture(colorMappingFunction, opacity);

            // voxel is occluded
            if(src.a >= 0.2) return 1.0;
        }

        pos += step;

        // out of bounds
        if (any(lessThan(pos, vec3(0.0))) || any(greaterThan(pos, vec3(1.0))))
            return 0.0;
    }

    return 0.0;
}

// direct light, ambient occlusion and indirect light for one sample, world space
vec3 volumetricShading(vec3 albedo, vec3 pos, vec4 gradient)
{
    vec3 toLight = normalize(-light.direction);
    vec3 worldPos = (ciModelMatrix * vec4(pos, 1.0)).xyz;
    vec3 toEye = normalize(eyePosition - worldPos);

    // surface normal points against the gradient, toward lower density. Shading
    // fades from a surface (lambert + specular) to a scattering medium as the
    // gradient magnitude drops, where the normal is noise
    float magnitude = gradient.w;
    float surface = gradientShading * smoothstep(0.005, 0.06, magnitude);
    vec3 normal = normalize(ciModelMatrixInverseTranspose * -gradient.xyz / max(magnitude, 1e-5));

    float lambert = max(dot(normal, toLight), 0.0);
    float diffuse = mix(1.0, lambert, surface);
    vec3 halfway = normalize(toLight + toEye);
    float specular = surface * specularStrength * (shininess + 8.0) / 25.13274 *
                     pow(max(dot(normal, halfway), 0.0), shininess) * step(0.0, lambert);

    float shadow = volumetricShadows ? texture(lightVolume, pos + lightOffset).r : 1.0;
    vec4 ambient = volumeOcclusion || indirectLight ? texture(ambientVolume, pos) : vec4(0.0, 0.0, 0.0, 1.0);
    // a point on an open flat surface sees half of the directions blocked, that counts as unoccluded
    float visibility = volumeOcclusion ? min(2.0 * ambient.a, 1.0) : 1.0;
    vec3 bounce = indirectLight ? ambient.rgb * indirectStrength : vec3(0.0);

    return albedo * (light.diffuse * (diffuse * shadow + bounce) + light.ambient * ambientStrength * visibility) +
           light.diffuse * specular * shadow;
}

void main(void)
{
    vec2 texC = position.xy / position.w;
    texC.x = 0.5 * texC.x + 0.5;
    texC.y = 0.5 * texC.y - 0.5;

    vec3 front = texture(cubeFront, texC).xyz;
    vec3 back = texture(cubeBack, texC).xyz;

    vec3 dir = normalize(back - front);
    float rayLength = length(back - front);
    vec3 pos = front;

    vec4 dst = vec4(0, 0, 0, 0);
    vec4 src = vec4(0, 0, 0, 0);

    vec4 value = vec4(0);

    vec3 step = dir * stepSize;
    float stepLength = length(step);

    // jitter ray starting position to reduce artifacts
    float travelled = stepLength * texture(bakedNoise, gl_FragCoord.xy / 256).x;
    pos += dir * travelled;

    for(int i = 0; i < iterations && travelled <= rayLength; i++)
    {
        value.a = texture(volume, pos).x;

        if(value.a >= threshold.x && value.a <= threshold.y)
        {
            float aOcclusion = 1.0;
            // assigned color from transfer function for this density
            src = texture(colorMappingFunction, value.a);

            // gradient value
            vec4 gradient = texture(gradients, pos);
            // outward normal, against the gradient toward lower density
            value.xyz = -gradient.xyz / max(length(gradient.xyz), 1e-5);
            vec3 wsNormal = normalize(ciModelMatrixInverseTranspose * value.xyz);

            // style transfer, view space calculation
            vec3 eye = normalize((ciModelView * vec4(pos, 1.0))).xyz;
            vec3 vsNormal = normalize(ciNormalMatrix * value.xyz);
            src *= styleMapping(eye, vsNormal, value.a);

            // opacity correction
            src.a = 1 - pow((1 - src.a), stepScale / 0.5);

            if(volumetricLighting)
            {
                src.rgb = volumetricShading(src.rgb, pos, gradient);
            }
            else
            {
                if(ambientOcclusion)
                {
                    aOcclusion = texelFetch(volumeAO, ivec2(gl_FragCoord.xy), 0).x;
                }

                if(diffuseShading)
                {
                    // diffuse shading + fake ambient light
                    vec3 lightDir = normalize(-light.direction);
                    float lambert = max(dot(wsNormal, lightDir), 0.0);
                    vec3 diffuse = light.diffuse * lambert * src.rgb;
                    vec3 ambient = light.ambient * src.rgb;
                    src.rgb = aOcclusion * ambient + diffuse;
                }
                else
                {
                    src.rgb *= aOcclusion;
                }
            }

            // front to back blending
            src.rgb *= src.a;
            dst = (1.0 - dst.a) * src + dst;

            // optimization: break from loop on high enough alpha value
            if(dst.a >= 0.95)
                break;
        }

        pos += step;
        travelled += stepLength;
    }

    oShadow = vec4(0.0);

    if(raycastShadows && !volumetricLighting)
    {
        // the ray marches in texture space, toward the light
        oShadow = vec4(voxelOcclusion(pos, normalize(lightOffset)) * 0.5);
    }

    oColor = dst;
    // store normal and position in view-space
    oNormal = vec4(ciNormalMatrix * value.xyz, 1.0);
    oPosition = vec4((ciModelView * vec4(pos, 1.0)).xyz, 1.0);
}
