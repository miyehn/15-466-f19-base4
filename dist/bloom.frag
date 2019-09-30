#version 330

in vec2 TexCoords;
uniform sampler2D image;
out vec4 displayColor;

void main() {
  displayColor = texture(image, TexCoords);
}
