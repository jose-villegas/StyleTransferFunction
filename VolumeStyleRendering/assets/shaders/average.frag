#version 420
layout(binding=0) uniform sampler2D source;

uniform vec2 texelSize;

in vec2 uvs;
out vec4 oColor;

void main() {
    vec4 result = vec4(0.0);

    for (int x = -2; x < 2; ++x) 
    {
        for (int y = -2; y < 2; ++y) 
        {
            vec2 offset = vec2(float(x), float(y)) * texelSize;
            result += texture(source, uvs + offset);
        }
    }

    oColor = result / 16.0;
}