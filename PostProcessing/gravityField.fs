#version 150 core

out vec4 FragColor;
uniform vec2 gravity;

void main()
{
    FragColor = vec4(gravity, 0.0, 0.0);
}
