#version 400
uniform sampler2D cubeFront;
uniform sampler2D cubeBack;
uniform sampler3D volume;
uniform float stepSize;
uniform int iterations;
in vec4 pos;
out vec4 fragmentColor;

void main(void)
{
    vec2 texC = pos.xy / pos.w;
    texC.x = 0.5 * texC.x + 0.5;
    texC.y = -0.5 * texC.y + 0.5;

    vec3 front = texture(cubeFront, texC).xyz;
    vec3 back = texture(cubeBack, texC).xyz;
    
    vec3 dir = normalize(back - front);
    vec4 pos = vec4(front, 0);

    vec4 dst = vec4(0, 0, 0, 0);
    vec4 src = vec4(0, 0, 0, 0);

    float value = 0;

    vec3 step = dir * stepSize;

    for(int i = 0; i < iterations; i++)
    {
        pos.w = 0;
        value = texture(volume, pos.xyz).r;

        src = vec4(value);
        src.a *= 0.5f; // alpha to make more visible

        src.rgb *= src.a;
        dst = (1.0f - dst.a) * src + dst;

        if(dst.a >= 0.95f) 
            break;

        pos.xyz += step;

        // out of bounds
        if (pos.x > 1.0f || pos.y > 1.0f || pos.z > 1.0f) 
            break;
    }

    fragmentColor = dst;
}