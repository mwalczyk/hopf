#version 460

uniform bool u_display_shadows;
layout(location = 0) uniform sampler2D u_depth_map;

layout(location = 0) out vec4 o_color;

in VS_OUT
{
    vec3 color;
    vec4 light_space_position;
} fs_in;

void main() 
{	
	float shadow = 0.0;
	if (u_display_shadows)
	{
		// Transform from light clip space to NDC
		vec3 projection_space_coordinates = fs_in.light_space_position.xyz / fs_in.light_space_position.w;

		// Transform NDC coordinates from -1..1 to 0..1
		projection_space_coordinates = projection_space_coordinates * 0.5 + 0.5;

		// Grab the current fragment depth
		float current = projection_space_coordinates.z;

		// Filtering
		const float bias = 0.005;
		const vec2 texel_size = 1.0 / textureSize(u_depth_map, 0);
		const int kernel_steps = 3;
		for(int x = -kernel_steps; x <= kernel_steps; ++x)
		{
		    for(int y = -kernel_steps; y <= kernel_steps; ++y)
		    {
		        float percentage_close = texture(u_depth_map, projection_space_coordinates.xy + vec2(x, y) * texel_size).r; 
		        shadow += current - bias > percentage_close ? 1.0 : 0.0;        
		    }    
		}

		// Normalize based on number of steps
		shadow /= pow(kernel_steps * 2.0 + 1.0, 2.0);

		// Prevent shadows from being 100% black
		shadow = min(shadow, 0.2);
	}

	//o_color = vec4(vec3(depth_read), 1.0);
	o_color = vec4(fs_in.color * (1.0 - shadow), 1.0);
}