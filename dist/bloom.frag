#version 330

in vec2 TexCoords;
uniform sampler2D IMG;
uniform int TASK; // 0: blur horizontally; 1: blur vertically; 2: combine result and draw to screen
out vec4 fragColor;

void main() {
  if (TASK == 0 || TASK == 1) fragColor = vec4(0.9, 0.9, 0.9, 1) * texture(IMG, TexCoords);
  else {
    fragColor = texture(IMG, TexCoords);
  }
}
