#version 410

layout (location = 0) in vec2 vertex_position;
layout (location = 1) in vec2 texture_mapping;

out vec2 texture_coords;
uniform float layer_z;
uniform float tx;
uniform float ty;
uniform mat4 model;
uniform mat4 projection;

void main () {
	texture_coords = texture_mapping;
	gl_Position = projection * model * vec4(position, 1.0);
    gl_Position = projection * model * vec4(vertex_position.x,
                  vertex_position.y,
                  layer_z, 1.0);
} 