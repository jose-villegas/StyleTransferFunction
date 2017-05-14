#version 420
layout(binding=0) uniform sampler2D colorBuffer;

uniform vec2 pixelSize;

in vec2     uvs;
out vec4 	oColor;

void main( void )
{
	float fxaaSpanMax	= 8.0;
	float fxaaReduceMul	= 1.0 / fxaaSpanMax;
	float fxaaReduceMin	= 1.0 / 128.0;

	vec3 rgbNW	= texture( colorBuffer, uvs + vec2( -1.0, -1.0 ) * pixelSize ).xyz;
	vec3 rgbNE	= texture( colorBuffer, uvs + vec2(  1.0, -1.0 ) * pixelSize ).xyz;
	vec3 rgbSW	= texture( colorBuffer, uvs + vec2( -1.0,  1.0 ) * pixelSize ).xyz;
	vec3 rgbSE	= texture( colorBuffer, uvs + vec2(  1.0,  1.0 ) * pixelSize ).xyz;
	vec3 rgbM	= texture( colorBuffer, uvs ).xyz;

	vec3 luma = vec3( 0.299, 0.587, 0.114 );
	float lumaNW = dot(rgbNW, luma);
	float lumaNE = dot(rgbNE, luma);
	float lumaSW = dot(rgbSW, luma);
	float lumaSE = dot(rgbSE, luma);
	float lumaM  = dot(rgbM,  luma);
	
	float lumaMin = min(lumaM, min(min(lumaNW, lumaNE), min(lumaSW, lumaSE)));
	float lumaMax = max(lumaM, max(max(lumaNW, lumaNE), max(lumaSW, lumaSE)));
	
	vec2 dir;
	dir.x = -((lumaNW + lumaNE) - (lumaSW + lumaSE));
	dir.y =  ((lumaNW + lumaSW) - (lumaNE + lumaSE));

	float dirReduce = max((lumaNW + lumaNE + lumaSW + lumaSE) *
						(0.25 * fxaaReduceMul), fxaaReduceMin);
	
	float rcpDirMin = 1.0 / (min(abs(dir.x), abs(dir.y)) + dirReduce);
	dir = min(vec2(fxaaSpanMax), max(vec2(-fxaaSpanMax), dir * rcpDirMin)) * pixelSize;

	vec3 color0 = 0.5 * (
		texture(colorBuffer, uvs + dir * (1.0 / 3.0 - 0.5)).rgb +
		texture(colorBuffer, uvs + dir * (2.0 / 3.0 - 0.5)).rgb );
	vec3 color1 = color0 * 0.5 + 0.25 * (
		texture(colorBuffer, uvs + dir * -0.5).rgb +
		texture(colorBuffer, uvs + dir *  0.5).rgb );
	
	float lumaB = dot(color1, luma);

	vec4 color = vec4(color1, 1.0);

	if (lumaB < lumaMin || lumaB > lumaMax) 
    {
		color.rgb = color0;
	}

	oColor = color;
}