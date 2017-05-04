#version 420
layout(binding=0) uniform sampler2D hdrBuffer;

uniform float gamma;
uniform float exposure;

in vec2 uvs;
out vec4 fragmentColor;

void main()
{
    vec3 hdrColor = texture(hdrBuffer, uvs).rgb;
  
    // Exposure tone mapping
    vec3 mapped =  vec3(1.0) - exp(-hdrColor * exposure);
    // Gamma correction 
    mapped = pow(mapped, vec3(1.0 / gamma));
  
    fragmentColor = vec4(mapped, 1.0);
}