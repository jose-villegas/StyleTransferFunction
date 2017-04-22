#version 400
uniform mat4 ciModelViewProjection;
layout(location = 0) in vec3 position;
out vec3 Color;

void main(void)
{
    gl_Position = ciModelViewProjection * vec4(position, 1);
    Color = position;
}