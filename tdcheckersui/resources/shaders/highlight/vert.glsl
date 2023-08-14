#version 430 core


layout (location = 0) in vec3 pos;
layout (location = 1) in vec2 tex;

uniform mat4 perspective;
uniform mat4 world;

void main()
{
    gl_Position = perspective * world * vec4(pos, 1.0);
}