#version 400
uniform mat4 ciModelViewProjection;
layout(location = 0) in vec3 position;
out vec4 pos;

void main(void)
{
    gl_Position = ciModelViewProjection * vec4(position, 1);
    pos = gl_Position;
}