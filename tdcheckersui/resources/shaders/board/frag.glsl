#version 430 core

out vec4 color;
in vec2 index;

void main()
{
    // this is for drawing the checkboard

    int widthi = min(7, int(floor(index.x * 8.0)));
    int heighti = min(7, int(floor(index.y * 8.0)));
    int isblack = ((widthi + heighti) % 2);

    vec4 black = vec4(215.0, 186.0, 137.0, 256.0) / 256.0;
    vec4 red = vec4(152.0, 107.0, 65.0, 256.0) / 256.0;

    color = isblack * black + (1 - isblack) * red;
}
