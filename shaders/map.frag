#version 120

uniform sampler2DRect tex0;
uniform vec2 resolution;
uniform float nearLimit;
uniform float farLimit;

float mapFunc(float val, float min1, float max1, float min2, float max2, bool bClamp){
	float pct = (val - min1) / (max1 - min1);

	float mapped = pct * (max2 - min2) + min2;
	if(bClamp) mapped = clamp(mapped, min2, max2);

	return mapped;
}

void main(){

	//get normalized coords
	vec4 depth = texture2DRect( tex0, gl_FragCoord.xy);
	float mapped = mapFunc(depth.r, nearLimit, farLimit, 1.0f, 0.0f, true);

	//make empty spaces black
	if(depth.r < 0.001) {
		mapped = 0.0f;
	}
	
	gl_FragColor = vec4(vec3(mapped), 1.0f); 

}