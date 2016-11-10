#version 150 core

out vec4 FragColor;
uniform sampler2D VelocityTexture;
uniform vec2 InverseSize;

void main()
{
	vec2 fragCoord = gl_FragCoord.xy;

	vec2 u = texture(VelocityTexture, fragCoord).xy;

	if (fragCoord.x > 400 && fragCoord.x < 450 && fragCoord.y > 500)
	{
		FragColor = vec4(u + vec2(0.0, 100.1), 0, 0);
	}
	else
	{
	    FragColor = vec4(u - vec2(0.0, 5.1), 0.0, 0.0);
	}
}
