#version 460

uniform float u_time;
uniform mat4 u_projection;
uniform mat4 u_view;
uniform mat4 u_model;

layout(location = 0) in vec3 i_position;
layout(location = 1) in vec3 i_color;
layout(location = 2) in vec2 i_texture_coordinates;

out VS_OUT
{
    vec3 color;
    vec3 position;
} vs_out;

void main() 
{
    gl_PointSize = 12.0;

    vec4 position_world_space = u_model * vec4(i_position, 1.0);
    gl_Position = u_projection * u_view * position_world_space;

    vs_out.color = i_color;
    vs_out.position = position_world_space.xyz;
}