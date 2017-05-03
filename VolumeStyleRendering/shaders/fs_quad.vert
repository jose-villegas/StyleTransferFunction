#version 420
uniform mat4 ciModelViewProjection;

layout(location = 0) in vec4 position;
layout(location = 1) in vec2 texCoord;

out vec2 uvs;

void main()
{
    // fs quad uv coords
    uvs = texCoord;
    // final drawing pos
    gl_Position = ciModelViewProjection * position;
}