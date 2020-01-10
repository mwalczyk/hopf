#version 460

uniform float u_time;

uniform mat4 u_projection;
uniform mat4 u_view;
uniform mat4 u_model;

layout(location = 0) in vec3 i_position;
layout(location = 1) in vec3 i_color;
layout(location = 2) in vec2 i_texture_coordinates;

out vec3 vs_color;

mat4 rotation(vec3 axis, float angle)
{
    axis = normalize(axis);
    float s = sin(angle);
    float c = cos(angle);
    float oc = 1.0 - c;
    
    return mat4(oc * axis.x * axis.x + c,           oc * axis.x * axis.y - axis.z * s,  oc * axis.z * axis.x + axis.y * s,  0.0,
                oc * axis.x * axis.y + axis.z * s,  oc * axis.y * axis.y + c,           oc * axis.y * axis.z - axis.x * s,  0.0,
                oc * axis.z * axis.x - axis.y * s,  oc * axis.y * axis.z + axis.x * s,  oc * axis.z * axis.z + c,           0.0,
                0.0,                                0.0,                                0.0,                                1.0);
}

void main() 
{
    gl_Position = u_projection * u_view * u_model * vec4(i_position, 1.0);

    vs_color = i_color;
}