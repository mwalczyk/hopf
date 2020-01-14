#version 460

uniform bool u_alpha;

layout(location = 0) out vec4 o_color;

in VS_OUT
{
    vec3 color;
} fs_in;

void main() 
{	
	//vec2 point_coordinates = gl_PointCoord - vec2(0.5);
	//if(length(point_coordinates) > 0.5) discard;

	o_color = vec4(fs_in.color, u_alpha ? 0.5 : 1.0);
}