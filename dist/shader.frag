#version 330

uniform sampler2D TEX;
in vec3 position;
in vec3 normal;
in vec4 color;
in vec2 texCoord;
in float depth;
in float height;
out vec4 fragColor;

vec4 over(vec4 elem, vec4 canvas) {
  vec4 elem_ = vec4(elem.rgb * elem.a, elem.a);
  float ca = 1 - (1-elem_.a) * (1-canvas.a);
  float cr = (1-elem_.a) * canvas.r + elem_.r;
  float cg = (1-elem_.a) * canvas.g + elem_.g;
  float cb = (1-elem_.a) * canvas.b + elem_.b;
  return vec4(cr, cg, cb, ca);
}

void main() {
	vec3 n = normalize(normal);
	vec3 l = normalize(vec3(0.1, 0.1, 1.0));
	vec4 albedo = texture(TEX, texCoord) * color;

	// simple hemispherical lighting model:
	vec3 light = mix(vec3(0.0,0.0,0.1), vec3(1.0,1.0,0.95), dot(n,l)*0.5+0.5);
	fragColor = vec4(light*albedo.rgb, albedo.a);

  // overlay fog color on top (as a function of depth)
  float depth_ = depth;
  depth_ = min(depth_, 200);
  depth_ = max(depth_, 0.1);
  float fog_extent = (depth_-0.1) / 199.9;
  vec4 fog = vec4(0.5, 0.56, 0.6, fog_extent);
  fragColor = over(fog, fragColor);
  
  // overlay height color (as a function of height and depth)
  // first get the overlay color from height
  vec4 height_col;
  float height_extent;
  float height_ = height;
  height_ = min(height_, 170);
  height_ = max(height_, 10);
  if (height_ < 90) {
    float height_lo = (height_ - 10) / 80;
    height_col = vec4(0.1,0.1,0.1,1-height_lo);
  } else {
    float height_hi = (170 - height_) / 80;
    height_col = vec4(0.8,0.8,0.8,1-height_hi);
  }
  // then tweak it according to depth
  float height_overlay_extent = min((depth_-0.1) / 40, 1);
  height_col.a = mix(0, height_col.a, height_overlay_extent);
  fragColor = over(height_col, fragColor);
  

}
