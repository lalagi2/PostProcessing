#version 330 core
layout (location = 0) out vec4 fragColor;
layout (location = 1) out vec4 brightColor;

uniform vec3 objectColor;
uniform vec3 lightColor;

void main()
{
    fragColor = vec4(lightColor * objectColor, 1.0f);

	float brightness = dot(fragColor.rgb, vec3(0.2126, 0.7152, 0.0722));
    if(brightness > 1.0)
        brightColor = vec4(fragColor.rgb, 1.0);
}