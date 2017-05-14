#version 420
layout(binding=0) uniform sampler2D color0;

in vec2 uvs;
out vec4 oColor;

void main()
{
    vec4 srcColor = texture(color0, uvs);
    oColor = 1.0 - srcColor;
}