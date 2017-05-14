#pragma once
#include <cinder/CinderMath.h>

class Light
{
public:
    glm::vec3 direction;
    glm::vec3 diffuse;
    glm::vec3 ambient;

    Light() : direction(0.0f, 0.0f, 1.0f), diffuse(1.0f, 1.0f, 1.0f), ambient(0.1f, 0.1f, 0.1f) {}
};
