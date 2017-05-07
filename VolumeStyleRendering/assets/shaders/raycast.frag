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

uniform mat3 ciModelMatrixInverseTranspose;
uniform Light light;

uniform ivec2 threshold;
uniform vec3 stepSize;
uniform int iterations;
uniform bool diffuseShading;
uniform float stepScale;
uniform float gamma;
uniform float exposure;

in vec4 position;

layout (location=0) out vec4 oColor;

// Spheremap Transform for normal encoding. Used in Cry Engine 3, presented by 
// Martin Mittring in "A bit more Deferred", p. 13
// http://www.crytek.com/sites/default/files/A_bit_more_deferred_-_CryEngine3.ppt
vec3 decode (vec2 enc)
{
    vec3 n;
    n.z = dot(enc, enc) * 2.0 - 1.0;
    n.xy = normalize(enc.xy) * sqrt(1.0 - n.z * n.z);
    return n;
}

vec3 ACESFilm( vec3 x )
{
    float a = 2.51;
    float b = 0.03;
    float c = 2.43;
    float d = 0.59;
    float e = 0.14;
    return clamp((x*(a*x+b))/(x*(c*x+d)+e), 0.0, 1.0);
}

vec2 litsphere (vec3 eye, vec3 normal) 
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

vec4 stylemapping (vec3 eye, vec3 normal, float opacity)
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

void main(void)
{
    vec2 texC = position.xy / position.w;
    texC.x = 0.5 * texC.x + 0.5;
    texC.y = 0.5 * texC.y - 0.5;

    vec3 front = texture(cubeFront, texC).xyz;
    vec3 back = texture(cubeBack, texC).xyz;
    
    vec3 dir = normalize(back - front);
    vec4 pos = vec4(front, 0);

    vec4 dst = vec4(0, 0, 0, 0);
    vec4 src = vec4(0, 0, 0, 0);

    vec4 value = vec4(0);

    vec3 step = dir * stepSize;
    
    // jitter ray starting position to reduce artifacts
    pos.xyz += step * texture(bakedNoise, gl_FragCoord.xy / 256).x;

    for(int i = 0; i < iterations; i++)
    {
        pos.w = 0;
        value.a = texture(volume, pos.xyz).x;
        int isoValue = int(255.0f * value.a);

        if(isoValue >= threshold.x && isoValue <= threshold.y)
        {
            // assigned color from transfer function for this density
            src = texture(colorMappingFunction, value.a);

            // gradient value
            value.xyz = decode(texture(gradients, pos.xyz).xy);
            vec3 normal = normalize(ciModelMatrixInverseTranspose * value.xyz);

            // style transfer
            src *= stylemapping(pos.xyz, normal, value.a);

            // oppacity correction
            src.a = 1 - pow((1 - src.a), stepScale / 0.5);

            if(diffuseShading)
            {
                // diffuse shading + fake ambient light
                vec3 lightDir = normalize(-light.direction);
                float lambert = max(dot(normal, lightDir), 0.0);
                src.rgb = light.diffuse * lambert * src.rgb + light.ambient * src.rgb;
            }

            // front to back blending
            src.rgb *= src.a;
            dst = (1.0 - dst.a) * src + dst;

            // optimization: break from loop on high enough alpha value
            if(dst.a >= 0.95) 
                break;
        }

        pos.xyz += step;

        // out of bounds
        if (pos.x > 1.0 || pos.y > 1.0 || pos.z > 1.0) 
            break;
    }

    // Exposure tone mapping
    vec3 mapped = ACESFilm(dst.rgb * exposure);
    // Gamma correction 
    mapped = pow(mapped, vec3(1.0 / gamma));

    oColor = vec4(mapped, 1.0);
}