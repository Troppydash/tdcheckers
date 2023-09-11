
#version 430 core

out vec4 color;

// state = 0 means red, = 1 means black
uniform float state;

// for circle
in vec2 coords;

void main()
{
    vec3 red = vec3(0.8, 0.0, 0.0);
    vec3 black = vec3(0.2, 0.2, 0.2);
    vec3 piececolor = state * black + (1-state) * red;

    float E = 2.718281828;
    float c = 20000.0;
    // compute closeness
    float radius = 0.36;
    float closeness = (coords.x * coords.x + coords.y * coords.y) - radius * radius;
    if (closeness < 0.0)
    {
        closeness = 1.0;
    }
    else
    {
        closeness = pow(E, -c * closeness*closeness);
    }

    color = vec4(piececolor, closeness);
}