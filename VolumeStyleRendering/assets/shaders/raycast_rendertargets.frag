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

uniform mat4 ciModelView;
uniform mat3 ciNormalMatrix;
uniform mat3 ciModelMatrixInverseTranspose;
uniform Light light;

uniform vec2 threshold;
uniform vec3 stepSize;
uniform vec3 shadowStepSize;
uniform int iterations;
uniform bool diffuseShading;
uniform bool raycastShadows;
uniform bool ambientOcclusion;
uniform float stepScale;

in vec4 position;

layout (location=0) out vec4 oColor;
layout (location=1) out vec3 oNormal;
layout (location=2) out float oShadow;
layout (location=3) out vec3 oPosition;

// Spheremap Transform for normal encoding. Used in Cry Engine 3, presented by 
// Martin Mittring in "A bit more Deferred", p. 13
// http://www.crytek.com/sites/default/files/A_bit_more_deferred_-_CryEngine3.ppt
vec3 decode(vec2 enc)
{
    vec3 n;
    n.z = dot(enc, enc) * 2.0 - 1.0;
    n.xy = normalize(enc.xy) * sqrt(1.0 - n.z * n.z);
    return n;
}

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
        if (pos.x >= 1.0 || pos.y >= 1.0 || pos.z >= 1.0) 
            return 0.0;
    }

    return 0.0;
}

void main(void)
{
    vec2 texC = position.xy / position.w;
    texC.x = 0.5 * texC.x + 0.5;
    texC.y = 0.5 * texC.y - 0.5;

    vec3 front = texture(cubeFront, texC).xyz;
    vec3 back = texture(cubeBack, texC).xyz;
    
    vec3 dir = normalize(back - front);
    vec3 pos = front;

    vec4 dst = vec4(0, 0, 0, 0);
    vec4 src = vec4(0, 0, 0, 0);

    vec4 value = vec4(0);

    vec3 step = dir * stepSize;
    
    // jitter ray starting position to reduce artifacts
    pos += step * texture(bakedNoise, gl_FragCoord.xy / 256).x;

    for(int i = 0; i < iterations; i++)
    {
        value.a = texture(volume, pos).x;

        if(value.a >= threshold.x && value.a <= threshold.y)
        {
            float aOcclusion = 1.0;
            // assigned color from transfer function for this density
            src = texture(colorMappingFunction, value.a);

            // gradient value
            value.xyz = decode(texture(gradients, pos).xy);
            vec3 wsNormal = normalize(ciModelMatrixInverseTranspose * value.xyz);

            // style transfer, view space calculation
            vec3 eye = normalize((ciModelView * vec4(pos, 1.0)).xyz;
            vec3 vsNormal = normalize(ciNormalMatrix * value.xyz);
            src *= styleMapping(eye, vsNormal, value.a);

            // opacity correction
            src.a = 1 - pow((1 - src.a), stepScale / 0.5);

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

            // front to back blending
            src.rgb *= src.a;
            dst = (1.0 - dst.a) * src + dst;

            // optimization: break from loop on high enough alpha value
            if(dst.a >= 0.95) 
                break;
        }

        pos += step;

        // out of bounds
        if (pos.x > 1.0 || pos.y > 1.0 || pos.z > 1.0) 
            break;
    }

    if(raycastShadows)
    {
        // use world direction in this case since raycast is done in world space
        vec3 lightDir = normalize(-light.direction);
        oShadow = voxelOcclusion(pos, -lightDir) * 0.5;
    }

    oColor = dst;
    // store normal and position in view-space
    oNormal = ciNormalMatrix * value.xyz;
    oPosition = (ciModelView * vec4(pos, 1.0)).xyz;
}