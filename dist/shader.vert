#version 330

uniform mat4 OBJECT_TO_CLIP;
uniform mat4x3 OBJECT_TO_LIGHT;
uniform mat3 NORMAL_TO_LIGHT;
uniform vec4 CUSTOM_COL;
uniform vec3 ANCHOR_POS;
in vec4 Position;
in vec3 Normal;
in vec4 Color;
in vec2 TexCoord;
out vec3 position;
out vec3 normal;
out vec4 color;
out vec2 texCoord;
out float depth;
out float height;

bool is_magenta(vec4 c) {
  return(c.x==1 && c.y==0 && c.z==1 && c.w==1);
}

void main() {
	gl_Position = OBJECT_TO_CLIP * Position;
  depth = gl_Position.z;
  height = ANCHOR_POS.z + Position.y;
	position = OBJECT_TO_LIGHT * Position;
	normal = NORMAL_TO_LIGHT * Normal;
	color = is_magenta(CUSTOM_COL) ? Color : CUSTOM_COL;
	texCoord = TexCoord;
}
