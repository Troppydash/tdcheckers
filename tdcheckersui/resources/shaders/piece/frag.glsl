
#version 430 core

out vec4 color;

// state = 0 means red, = 1 means black
uniform float state;

// for circle
in vec2 coords;

void main()
{
    vec4 red = vec4(0.8, 0.0, 0.0, 1.0);
    vec4 black = vec4(0.2, 0.2, 0.2, 1.0);

    float radius = 0.36;
    if ((coords.x * coords.x + coords.y * coords.y) > radius * radius)
    {
        discard;
    }

    color = state * black + (1-state) * red;
}