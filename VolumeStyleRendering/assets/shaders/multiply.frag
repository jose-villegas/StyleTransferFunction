#version 420
layout(binding=0) uniform sampler2D color0;
layout(binding=1) uniform sampler2D color1;

in vec2 uvs;
out vec4 oColor;

void main()
{
    vec4 srcColor0 = texture(color0, uvs);
    vec4 srcColor1 = texture(color1, uvs);
    oColor = srcColor0 * srcColor1;
}