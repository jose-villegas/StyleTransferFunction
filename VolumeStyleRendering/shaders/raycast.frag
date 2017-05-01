#version 420
layout(binding=0) uniform sampler2D cubeFront;
layout(binding=1) uniform sampler2D cubeBack;
layout(binding=2) uniform sampler3D volume;
layout(binding=3) uniform sampler3D gradients;
layout(binding=4) uniform sampler1D transferFunction;
uniform ivec2 threshold;
uniform vec3 stepSize;
uniform int iterations;
uniform bool diffuseShading;
in vec4 position;
out vec4 fragmentColor;

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

void main(void)
{
    vec3 L = vec3(0, 1, 1);

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

    for(int i = 0; i < iterations; i++)
    {
        pos.w = 0;
        value.a = texture(volume, pos.xyz).x;
        int isoValue = int(255.0f * value.a);

        if(isoValue >= threshold.x && isoValue <= threshold.y)
        {
            // gradient value
            value.xyz = decode(texture(gradients, pos.xyz).xy);

            // assigned color from transfer function for this density
            src = texture(transferFunction, value.a);

            if(diffuseShading)
            {
                // diffuse shading + fake ambient light
                float s = dot(value.xyz, L);
                src.rgb = s * src.rgb + 0.1f * src.rgb;
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

    fragmentColor = dst;
}