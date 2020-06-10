#version 330 core

in vec3 _colour;
in vec2 _tex_coords;

out vec4 FragmentColour;

uniform sampler2D texture_data;

void main() {
  vec4 c = texture(texture_data, _tex_coords);
  FragmentColour = vec4(c.r*50, c.r*150, c.r*20, 255);
}
