#version 420
layout(binding=0) uniform sampler2D source;

uniform int blurIntensity;
uniform vec2 blurDirection;

in vec2 uvs;
out vec4 oColor;

vec4 blur13() 
{
	vec4 color = vec4(0.0);
	vec2 off1 = vec2(1.411764705882353) * blurDirection;
	vec2 off2 = vec2(3.2941176470588234) * blurDirection;
	vec2 off3 = vec2(5.176470588235294) * blurDirection;
	color += texture(source, uvs) * 0.1964825501511404;
	color += texture(source, uvs + off1) * 0.2969069646728344;
	color += texture(source, uvs - off1) * 0.2969069646728344;
	color += texture(source, uvs + off2) * 0.09447039785044732;
	color += texture(source, uvs - off2) * 0.09447039785044732;
	color += texture(source, uvs + off3) * 0.010381362401148057;
	color += texture(source, uvs - off3) * 0.010381362401148057;
	return color;
}

vec4 blur5() 
{
	vec4 color = vec4(0.0);
	vec2 off1 = vec2(1.3333333333333333) * blurDirection;
	color += texture(source, uvs) * 0.29411764705882354;
	color += texture(source, uvs + off1) * 0.35294117647058826;
	color += texture(source, uvs - off1) * 0.35294117647058826;
	return color; 
}

vec4 blur9() {
	vec4 color = vec4(0.0);
	vec2 off1 = vec2(1.3846153846) * blurDirection;
	vec2 off2 = vec2(3.2307692308) * blurDirection;
	color += texture(source, uvs) * 0.2270270270;
	color += texture(source, uvs + off1) * 0.3162162162;
	color += texture(source, uvs - off1) * 0.3162162162;
	color += texture(source, uvs + off2) * 0.0702702703;
	color += texture(source, uvs - off2) * 0.0702702703;
	return color;
}

void main()
{
	if(blurIntensity == 1) 
		oColor = blur5();
	else if(blurIntensity == 2) 
		oColor = blur9();
	else if(blurIntensity == 3) 
		oColor = blur13();
	else 
		oColor = blur5();
}