#version 330

in vec2 TexCoords;
uniform sampler2D IMG;
uniform sampler2D FRAME; // only used when TASK==2: the originally rendered frame
uniform sampler2D HIGHLIGHT;
uniform int TASK; // 0: blur horizontally; 1: blur vertically; 2: combine result and draw to screen
out vec4 fragColor;

bool is_light(vec4 col) {
  return col.a==1 && (col.r==1 || col.g==1 || col.b==1);
}

vec4 over(vec4 elem, vec4 canvas) {
  vec4 elem_ = vec4(elem.rgb * elem.a, elem.a);
  float ca = 1 - (1-elem_.a) * (1-canvas.a);
  float cr = (1-elem_.a) * canvas.r + elem_.r;
  float cg = (1-elem_.a) * canvas.g + elem_.g;
  float cb = (1-elem_.a) * canvas.b + elem_.b;
  return vec4(cr, cg, cb, ca);
}

float weight[5] = float[] (0.227027, 0.1945946, 0.1216216, 0.054054, 0.016216);

void main() {
  if (TASK == 0 || TASK == 1) {
    vec2 tex_offset = 1.0 / textureSize(IMG, 0);
    vec4 result = texture(IMG, TexCoords) * weight[0];
    if (TASK == 0) { // horizontal blur
      for(int i = 1; i < 5; ++i) {
        result += texture(IMG, TexCoords + vec2(tex_offset.x * i, 0)) * weight[i];
        result += texture(IMG, TexCoords - vec2(tex_offset.x * i, 0)) * weight[i];
      }
    } else if (TASK == 1) { // vertical blur
      for(int i = 1; i < 5; ++i) {
        result += texture(IMG, TexCoords + vec2(0, tex_offset.y * i)) * weight[i];
        result += texture(IMG, TexCoords - vec2(0, tex_offset.y * i)) * weight[i];
      }
    }
    fragColor = result;
  } else { // combine with first pass result
    vec4 firstpass = texture(FRAME, TexCoords);
    vec4 highlight = texture(HIGHLIGHT, TexCoords);
    vec4 tex = texture(IMG, TexCoords);
    if (is_light(highlight)) {
      fragColor = firstpass + tex * 0.5;
    } else {
      fragColor = firstpass + tex;
    }
  }
}
