#version 150 core

in vec2 UV;
out vec4 FragColor;
uniform sampler2D Sampler;
uniform vec3 FillColor;
uniform vec2 Scale;

void main()
{
    vec4 t = texture(Sampler, gl_FragCoord.xy * Scale);
    FragColor = abs(t);
}
