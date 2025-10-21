#pragma once
const char laplacian_sharp[] = R"!!(
vec2 fTextureCoord = gl_FragCoord.xy / iResolution.xy;
uniform float 	  degree;
vec3 sharp(vec2 coord){
		float Sx[9];
		Sx[6] = 0.; Sx[7] = -1.; Sx[8] = 0.;
		Sx[3] = -1.; Sx[4] = 5.; Sx[5] = -1.;
		Sx[0] = 0.; Sx[1] = -1.; Sx[2] = 0.;
		float dx = 1.0 / float(iResolution.x);
		float dy = 1.0 / float(iResolution.y);
		vec3 curColor = texture2D( iChannel[0], coord ).rgb ;
		vec3 s10 = texture2D( iChannel[0],
		coord + vec2(-dx,0.0) ).rgb ;
		vec3 s01 = texture2D( iChannel[0],
		coord + vec2(0.0,dy) ).rgb ;
		vec3 s21 = texture2D( iChannel[0],
		coord + vec2(0.0,-dy) ).rgb ;
		vec3 s12 = texture2D( iChannel[0],
		coord + vec2(dx, 0.0) ).rgb ;
		vec3 outputcolor = ((Sx[4]-1)*curColor + s10 * Sx[3] + s01 * Sx[7]
					+ s21 * Sx[1] + s12 * Sx[5])*degree/100 + curColor;
		return outputcolor;
}
void main()
{
	vec3 outcolor = sharp(fTextureCoord);
	gl_FragColor = vec4(outcolor, texture2D( iChannel[0], fTextureCoord ).a);
}
)!!";

const char  histogramRes[] = R"!!(
void mainImage( out vec4 fragColor, in vec2 fragCoord ) {
	vec2 uv = fragCoord / iResolution.xy;
	vec3 srcColor = texture2D(iChannel[0], uv).rgb;

	int tempr = int(srcColor.r*255);
	int tempg = int(srcColor.g*255);
	int tempb = int(srcColor.b*255);
	
	float lutdatar = texelFetch (iChannel[1], ivec2 (tempr, 0),0).r;
	float lutdatag = texelFetch (iChannel[1], ivec2 (tempg, 0),0).r;
	float lutdatab = texelFetch (iChannel[1], ivec2 (tempb, 0),0).r;
	
	srcColor.r = lutdatar;
	srcColor.g = lutdatag;
	srcColor.b = lutdatab;

	fragColor = vec4(srcColor, texture2D(iChannel[0], uv).a);
}

void main() {
    mainImage(gl_FragColor, gl_FragCoord.xy);
}
)!!";