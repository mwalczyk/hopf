#version 460

layout(location = 0) out vec4 o_color;

in VS_OUT
{
    vec3 color;
    vec3 position;
} fs_in;

void main() 
{	
	// Calculate some very simple lighting for the central sphere and point cloud
	const vec3 light_position = vec3(2.0, 2.0, 3.0);
    const vec3 normal = normalize(fs_in.position);
    float intensity = max(0.75, clamp(dot(normal, light_position), 0.0, 1.0));

	o_color = vec4(fs_in.color * intensity, 1.0);
}