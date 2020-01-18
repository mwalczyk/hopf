#version 460

uniform mat4 u_light_space_matrix;
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
    vec4 light_space_position;
} vs_out;


void main() 
{
    gl_PointSize = 4.0;
    gl_Position = u_projection * u_view * u_model * vec4(i_position, 1.0);

    vs_out.color = i_color;
    vs_out.light_space_position = u_light_space_matrix * u_model * vec4(i_position, 1.0);
}