#pragma once

#include "GL.hpp"
#include "Load.hpp"
#include "Scene.hpp"

// can perform a few postprocessing tasks depending on what param is passed to TASK uniform
// - gaussian blur to ping-pong textures (0, 1)
// - combine blurred result with original frame -> bloom (2)
// - combine shadow layer with albedo layer (toon) + pixelate + maybe outline (3)
struct PostprocessingProgram {
  PostprocessingProgram();
  ~PostprocessingProgram();

  GLuint program = 0;

  // vert input location:
  GLuint Position_vec4 = -1U;

  // frag input locations:
  GLuint TASK_int = -1U;
  GLuint TEX0_tex = -1U;
  GLuint TEX1_tex = -1U;
  GLuint TEX2_tex = -1U;
  GLuint TEX_OFFSET_vec2 = -1U;

};

extern Load< PostprocessingProgram > postprocessing_program;
