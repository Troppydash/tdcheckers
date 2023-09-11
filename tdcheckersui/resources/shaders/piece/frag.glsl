
#version 430 core

out vec4 color;

// state = 0 means red, = 1 means black
uniform float state;
// 0 means no, 1 means yes
uniform float king;

// for circle
in vec2 coords;

float circle(vec2 coord, float radius)
{
    return sqrt(abs((coord.x * coord.x + coord.y * coord.y) - radius * radius));
}

float dist(vec2 coord, float radius)
{
    return (coord.x * coord.x + coord.y * coord.y) - radius * radius;
}

void main()
{
    vec3 red = vec3(0.8, 0.0, 0.0);
    vec3 black = vec3(0.2, 0.2, 0.2);
    vec3 piececolor = state * black + (1-state) * red;

    float E = 2.718281828;
    float c = 100.0;
    float radius = 0.36;

    // create piece shape
    float closeness = dist(coords, radius);
    float alpha = 1.0;
    if (closeness > 0.0)
    {
        float circ = circle(coords, radius);
        alpha = pow(E, -c * circ*circ);
    }

    // create king ring
    float c1 = circle(coords, radius);
    float ring = (pow(E, -c/3.0 * c1*c1));

    if (king == 0.0)
    {
        color = vec4(piececolor, alpha);
    }
    else
    {
        vec3 mixed = mix(piececolor, vec3(0.0, 1.0, 0.0), ring);
        color = vec4(mixed, alpha);

    }
}