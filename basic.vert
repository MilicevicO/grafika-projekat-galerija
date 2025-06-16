#version 330 core 

layout(location = 0) in vec3 inPos;
//layout(location = 1) in vec2 inTex; //Koordinate texture, propustamo ih u FS kao boje
//out vec2 chTex;

//uniform vec2 uPos;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;


void main()
{
	//gl_Position = vec4(inPos.x + uPos.x, inPos.y + uPos.y, 0.0, 1.0);
	gl_Position = projection * view * model * vec4(inPos, 1.0);


	//chTex = inTex;
}