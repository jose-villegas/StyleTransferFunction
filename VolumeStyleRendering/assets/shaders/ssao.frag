#version 420
layout(binding=0) uniform sampler2D gNormal;
layout(binding=1) uniform sampler2D gPosition;
layout(binding=2) uniform sampler2D ssaoNoise;

uniform vec3 ssaoKernel[64];
uniform ivec2 ciWindowSize;
uniform mat4 projectionMatrix;

// parameters
uniform float radius = 0.5;
uniform float bias = 0.025;
uniform float power = 1.0;

in vec2 uvs;
out vec4 oColor;

void main()
{
    vec3 pos = texture(gPosition, uvs).xyz;
    vec3 normal = texture(gNormal, uvs).xyz;
    vec3 randomVec = texture(ssaoNoise, uvs * 0.25 * ciWindowSize).xyz;

    // build tanget space matrix
    vec3 tangent   = normalize(randomVec - normal * dot(randomVec, normal));
    vec3 bitangent = cross(normal, tangent);
    mat3 TBN       = mat3(tangent, bitangent, normal);

    float occlusion = 0.0;

    for(int i = 0; i < 64; ++i)
    {
        // get sample position
        vec3 kernelSample = TBN * ssaoKernel[i];        // from tangent to view-space
        kernelSample = pos + kernelSample * radius; 
        
        vec4 offset = vec4(kernelSample, 1.0);
        offset = projectionMatrix * offset;             // from view to clip-space
        offset.xyz /= offset.w;                         // perspective divide
        offset.xyz  = offset.xyz * 0.5 + 0.5;           // transform to range 0.0 - 1.0

        float offsetDepth = texture(gPosition, offset.xy).z;  
        float rangeCheck = smoothstep(0.0, 1.0, radius / abs(pos.z - offsetDepth));
        occlusion += (offsetDepth >= kernelSample.z + bias ? 1.0 : 0.0) * rangeCheck;   
    }

    occlusion = 1.0 - (occlusion / 64);
    oColor = vec4(pow(occlusion, power)); 
}