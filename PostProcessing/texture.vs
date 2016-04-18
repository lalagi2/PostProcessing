#version 330 core

in vec3 position;
in vec3 color;
in vec2 texcoord;

out vec3 Color;
out vec2 Texcoord;
		
void main()
{
    Color = color;
    Texcoord = texcoord;
    gl_Position = vec4(-position.x, position.y, position.z, 1.0);
};