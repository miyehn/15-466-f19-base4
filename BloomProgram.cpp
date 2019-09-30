#include "BloomProgram.hpp"

#include "gl_compile_program.hpp"
#include "gl_errors.hpp"
#include <fstream>
#include "data_path.hpp"

// Scene::Drawable::Pipeline lit_color_texture_program_pipeline;


BloomProgram::BloomProgram() {
  //Compile vertex and fragment shaders using the convenient 'gl_compile_program' helper function:
  std::ifstream vertex_fs(data_path("bloom.vert"));
  std::string vert_content( 
      (std::istreambuf_iterator<char>(vertex_fs)), std::istreambuf_iterator<char>() );

  std::ifstream fragment_fs(data_path("bloom.frag"));
  std::string frag_content( 
      (std::istreambuf_iterator<char>(fragment_fs)), std::istreambuf_iterator<char>() );

  program = gl_compile_program(
    //vertex shader:
    vert_content
  ,
    //fragment shader:
    frag_content
  );
  //As you can see above, adjacent strings in C/C++ are concatenated.
  // this is very useful for writing long shader programs inline.

/*
  //look up the locations of vertex attributes:
  Position_vec4 = glGetAttribLocation(program, "Position");
  Normal_vec3 = glGetAttribLocation(program, "Normal");
  Color_vec4 = glGetAttribLocation(program, "Color");
  TexCoord_vec2 = glGetAttribLocation(program, "TexCoord");

  //look up the locations of uniforms:
  OBJECT_TO_CLIP_mat4 = glGetUniformLocation(program, "OBJECT_TO_CLIP");
  OBJECT_TO_LIGHT_mat4x3 = glGetUniformLocation(program, "OBJECT_TO_LIGHT");
  NORMAL_TO_LIGHT_mat3 = glGetUniformLocation(program, "NORMAL_TO_LIGHT");
  GLuint TEX_sampler2D = glGetUniformLocation(program, "TEX");

  //set TEX to always refer to texture binding zero:
  glUseProgram(program); //bind program -- glUniform* calls refer to this program now

  glUniform1i(TEX_sampler2D, 0); //set TEX to sample from GL_TEXTURE0

  glUseProgram(0); //unbind program -- glUniform* calls refer to ??? now
  */
}

BloomProgram::~BloomProgram() {
  glDeleteProgram(program);
  program = 0;
}

