#version 420
struct Light 
{
    vec3 direction;
    vec3 ambient;
    vec3 diffuse;
};

layout(binding=0) uniform sampler2D albedo;
layout(binding=1) uniform sampler2D normal;

uniform mat3 ciModelMatrixInverseTranspose;
uniform bool diffuseShading;
uniform Light light;

in vec2 uvs;

out vec4 fragmentColor;

void main()
{
    vec3 color = texture(albedo, uvs).rgb;

    // lambertian diffuse
    if(diffuseShading)
    {
        vec3 normal = texture(normal, uvs).xyz;
        vec3 lightDir = normalize(-light.direction);
        float lambert = max(dot(normal, lightDir), 0.0);
        color = light.diffuse * lambert * color + light.ambient * color;
    }

    // apply gamma correction
    float gamma = 2.2;
    color = pow(color, vec3(1.0 / gamma));

    fragmentColor = vec4(color, 1.0);
}