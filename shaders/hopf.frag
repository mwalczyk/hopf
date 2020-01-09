#version 460

layout(location = 0) out vec4 o_color;

in vec3 vs_color;

void main() 
{	
	o_color = vec4(vs_color, 1.0);
}