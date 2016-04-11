#version 330 core

in vec2 Texcoord;
out vec4 outColor;
uniform sampler2D text;

void main()
{
    outColor = texture(text, Texcoord);
};