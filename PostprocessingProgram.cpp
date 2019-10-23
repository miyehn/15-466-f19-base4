#include "PostprocessingProgram.hpp"

#include "gl_compile_program.hpp"
#include "gl_errors.hpp"
#include <fstream>
#include "data_path.hpp"

Load< PostprocessingProgram > postprocessing_program(LoadTagEarly, []() -> PostprocessingProgram const * {
	PostprocessingProgram *ret = new PostprocessingProgram();
  return ret;
});

PostprocessingProgram::PostprocessingProgram() {
  //Compile vertex and fragment shaders using the convenient 'gl_compile_program' helper function:
  std::ifstream vertex_fs(data_path("postprocessing.vert"));
  std::string vert_content( 
      (std::istreambuf_iterator<char>(vertex_fs)), std::istreambuf_iterator<char>() );

  std::ifstream fragment_fs(data_path("postprocessing.frag"));
  std::string frag_content( 
      (std::istreambuf_iterator<char>(fragment_fs)), std::istreambuf_iterator<char>() );

  program = gl_compile_program(
    //vertex shader:
    vert_content,
    //fragment shader:
    frag_content
  );
  //look up the locations of vert attributes:
  Position_vec4 = glGetAttribLocation(program, "Position");

  //look up the locations of uniforms (frag attributes):
  TASK_int = glGetUniformLocation(program, "TASK");
  TEX0_tex = glGetUniformLocation(program, "IMG");
  TEX1_tex = glGetUniformLocation(program, "FRAME");
  TEX2_tex = glGetUniformLocation(program, "HIGHLIGHT");
  TEX_OFFSET_vec2 = glGetUniformLocation(program, "TEX_OFFSET");

  assert(TASK_int != -1U);
  assert(TEX0_tex != -1U);
  assert(TEX0_tex != -1U);
  assert(TEX0_tex != -1U);
  assert(TEX_OFFSET_vec2 != -1U);
  // GLuint TEX_sampler2D = glGetUniformLocation(program, "TEX");

}

PostprocessingProgram::~PostprocessingProgram() {
  glDeleteProgram(program);
  program = 0;
}

