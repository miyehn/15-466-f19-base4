#include "Scene.hpp"

#include "gl_errors.hpp"
#include "read_write_chunk.hpp"

#include <glm/gtc/type_ptr.hpp>

#include <fstream>

//-------------------------


glm::mat4 Scene::Transform::make_local_to_parent() const {
  return glm::mat4( //translate
    glm::vec4(1.0f, 0.0f, 0.0f, 0.0f),
    glm::vec4(0.0f, 1.0f, 0.0f, 0.0f),
    glm::vec4(0.0f, 0.0f, 1.0f, 0.0f),
    glm::vec4(position, 1.0f)
  )
  * glm::mat4_cast(rotation) //rotate
  * glm::mat4( //scale
    glm::vec4(scale.x, 0.0f, 0.0f, 0.0f),
    glm::vec4(0.0f, scale.y, 0.0f, 0.0f),
    glm::vec4(0.0f, 0.0f, scale.z, 0.0f),
    glm::vec4(0.0f, 0.0f, 0.0f, 1.0f)
  );
}

glm::mat4 Scene::Transform::make_parent_to_local() const {
  glm::vec3 inv_scale;
  inv_scale.x = (scale.x == 0.0f ? 0.0f : 1.0f / scale.x);
  inv_scale.y = (scale.y == 0.0f ? 0.0f : 1.0f / scale.y);
  inv_scale.z = (scale.z == 0.0f ? 0.0f : 1.0f / scale.z);
  return glm::mat4( //un-scale
    glm::vec4(inv_scale.x, 0.0f, 0.0f, 0.0f),
    glm::vec4(0.0f, inv_scale.y, 0.0f, 0.0f),
    glm::vec4(0.0f, 0.0f, inv_scale.z, 0.0f),
    glm::vec4(0.0f, 0.0f, 0.0f, 1.0f)
  )
  * glm::mat4_cast(glm::inverse(rotation)) //un-rotate
  * glm::mat4( //un-translate
    glm::vec4(1.0f, 0.0f, 0.0f, 0.0f),
    glm::vec4(0.0f, 1.0f, 0.0f, 0.0f),
    glm::vec4(0.0f, 0.0f, 1.0f, 0.0f),
    glm::vec4(-position, 1.0f)
  );
}

glm::mat4 Scene::Transform::make_local_to_world() const {
  if (!parent) {
    return make_local_to_parent();
  } else {
    return parent->make_local_to_world() * make_local_to_parent();
  }
}
glm::mat4 Scene::Transform::make_world_to_local() const {
  if (!parent) {
    return make_parent_to_local();
  } else {
    return make_parent_to_local() * parent->make_world_to_local();
  }
}

//-------------------------

glm::mat4 Scene::Camera::make_projection() const {
  return glm::infinitePerspective( fovy, aspect, near );
}

//-------------------------

void Scene::draw(glm::uvec2 drawable_size, Camera const &camera) const {
  assert(camera.transform);
  glm::mat4 world_to_clip = camera.make_projection() * camera.transform->make_world_to_local();
  glm::mat4x3 world_to_light = glm::mat4x3(1.0f);
  draw(drawable_size, world_to_clip, world_to_light);
}

void Scene::draw(glm::uvec2 drawable_size, glm::mat4 const &world_to_clip, glm::mat4x3 const &world_to_light) const {

  glBindFramebuffer(GL_FRAMEBUFFER, firstpass_fbo);
  glViewport(0, 0, drawable_size.x/4.0f, drawable_size.y/4.0f);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  //Iterate through all drawables, sending each one to OpenGL:
  for (auto const &drawable : drawables) {
    //Reference to drawable's pipeline for convenience:
    Scene::Drawable::Pipeline const &pipeline = drawable.pipeline;

    //skip any drawables without a shader program set:
    if (pipeline.program == 0) continue;
    //skip any drawables that don't contain any vertices:
    if (pipeline.count == 0) continue;


    //Set shader program:
    glUseProgram(pipeline.program);

    //Set attribute sources:
    glBindVertexArray(pipeline.vao);

    //Configure program uniforms:

    //the object-to-world matrix is used in all three of these uniforms:
    assert(drawable.transform); //drawables *must* have a transform
    glm::mat4 object_to_world = drawable.transform->make_local_to_world();

    //OBJECT_TO_CLIP takes vertices from object space to clip space:
    if (pipeline.OBJECT_TO_CLIP_mat4 != -1U) {
      glm::mat4 object_to_clip = world_to_clip * object_to_world;
      glUniformMatrix4fv(pipeline.OBJECT_TO_CLIP_mat4, 1, GL_FALSE, glm::value_ptr(object_to_clip));
    }

    //the object-to-light matrix is used in the next two uniforms:
    glm::mat4x3 object_to_light = world_to_light * object_to_world;

    //OBJECT_TO_CLIP takes vertices from object space to light space:
    if (pipeline.OBJECT_TO_LIGHT_mat4x3 != -1U) {
      glUniformMatrix4x3fv(pipeline.OBJECT_TO_LIGHT_mat4x3, 1, GL_FALSE, glm::value_ptr(object_to_light));
    }

    //NORMAL_TO_CLIP takes normals from object space to light space:
    if (pipeline.NORMAL_TO_LIGHT_mat3 != -1U) {
      glm::mat3 normal_to_light = glm::inverse(glm::transpose(glm::mat3(object_to_light)));
      glUniformMatrix3fv(pipeline.NORMAL_TO_LIGHT_mat3, 1, GL_FALSE, glm::value_ptr(normal_to_light));
    }

    //set any requested custom uniforms:
    if (pipeline.set_uniforms) pipeline.set_uniforms();

    //set up textures:
    for (uint32_t i = 0; i < Drawable::Pipeline::TextureCount; ++i) {
      if (pipeline.textures[i].texture != 0) {
        glActiveTexture(GL_TEXTURE0 + i);
        glBindTexture(pipeline.textures[i].target, pipeline.textures[i].texture);
      }
    }

    //draw the object:
    glDrawArrays(pipeline.type, pipeline.start, pipeline.count);

    //un-bind textures:
    for (uint32_t i = 0; i < Drawable::Pipeline::TextureCount; ++i) {
      if (pipeline.textures[i].texture != 0) {
        glActiveTexture(GL_TEXTURE0 + i);
        glBindTexture(pipeline.textures[i].target, 0);
      }
    }
    glActiveTexture(GL_TEXTURE0);

  }

  glUseProgram(0);
  glBindVertexArray(0);
  GL_ERRORS();

  glUseProgram(post_processing_program);
  glBindVertexArray(trivial_vao);
  glBindBuffer(GL_ARRAY_BUFFER, trivial_vbo);
  glActiveTexture(GL_TEXTURE0);
  glViewport(0, 0, drawable_size.x, drawable_size.y);

  if (use_postprocessing) {

    bool horizontal = true, first_iteration = true;
    int amount = 6;
    for (unsigned int i = 0; i < amount; i++) {
      glBindFramebuffer(GL_FRAMEBUFFER, pingpong_fbo[horizontal]); 
      // set horizontal uniform
      GLuint loc = glGetUniformLocation(post_processing_program, "TASK");
      assert (loc != -1U);
      glUniform1i(loc, (int)horizontal);
      // bind input texture
      glBindTexture(
          GL_TEXTURE_2D, first_iteration ? colorBuffers[1] : pingpongBuffers[!horizontal]
      ); 
      glDrawArrays(GL_TRIANGLES, 0, 6);
      horizontal = !horizontal;
      if (first_iteration)
        first_iteration = false;
    }

    // ---- draw to screen
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    // set uniform so the shader draws the last pass
    GLuint loc = glGetUniformLocation(post_processing_program, "TASK");
    assert (loc != -1U);
    glUniform1i(loc, 2);
    // bind texture(s)
    glUniform1i(glGetUniformLocation(post_processing_program, "IMG"), 0);
    glUniform1i(glGetUniformLocation(post_processing_program, "FRAME"), 1);
    glUniform1i(glGetUniformLocation(post_processing_program, "HIGHLIGHT"), 2);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, pingpongBuffers[1]);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, colorBuffers[0]);
    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, colorBuffers[1]);

    glDrawArrays(GL_TRIANGLES, 0, 6);
    GL_ERRORS();

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
    glUseProgram(0);
  } else { // copy to screen without any post processing
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    // set uniform so the shader performs copy to screen directly
    GLuint loc = glGetUniformLocation(post_processing_program, "TASK");
    assert (loc != -1U);
    glUniform1i(loc, 3);
    // bind input
    glUniform1i(glGetUniformLocation(post_processing_program, "IMG"), 0);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, colorBuffers[0]);
    // draw
    glDrawArrays(GL_TRIANGLES, 0, 6);
    GL_ERRORS();
    // unbind things
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
    glUseProgram(0);
  }

}


void Scene::load(std::string const &filename,
  std::function< void(Scene &, Transform *, std::string const &) > const &on_drawable) {
  
  // ------ generate framebuffer for first pass
  glGenFramebuffers(1, &firstpass_fbo);
  glBindFramebuffer(GL_FRAMEBUFFER, firstpass_fbo);

  glGenTextures(2, colorBuffers);
  for (GLuint i=0; i<2; i++) {
    glBindTexture(GL_TEXTURE_2D, colorBuffers[i]);
    glTexImage2D(
      // ended up disabling high resolution draw so the program runs at a reasonable framerate...
      GL_TEXTURE_2D, 0, GL_RGBA, 200, 135, 0, GL_RGBA, GL_FLOAT, NULL    
    );
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glFramebufferTexture2D(
      GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0+i, GL_TEXTURE_2D, colorBuffers[i], 0    
    );
  }
  // setup associated depth buffer
  glGenRenderbuffers(1, &depthBuffer);
  glBindRenderbuffer(GL_RENDERBUFFER, depthBuffer);
  glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, 800, 540);
  glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, depthBuffer);

  glDrawBuffers(2, color_attachments);
  glBindFramebuffer(GL_FRAMEBUFFER, 0);
  assert(glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE);

  // ------ set up 2nd pass pipeline
  glGenVertexArrays(1, &trivial_vao);
  glBindVertexArray(trivial_vao);

  glGenBuffers(1, &trivial_vbo);
  glBindBuffer(GL_ARRAY_BUFFER, trivial_vbo);
  glBufferData(
      GL_ARRAY_BUFFER, 
      trivial_vector.size() * sizeof(float),
      trivial_vector.data(),
      GL_STATIC_DRAW);

  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3*sizeof(float), (void*)0);
  glEnableVertexAttribArray(0);

  glBindBuffer(GL_ARRAY_BUFFER, 0);
  glBindVertexArray(0);
  GL_ERRORS();

  // ------ ping pong framebuffers for gaussian blur
  glGenFramebuffers(2, pingpong_fbo);
  glGenTextures(2, pingpongBuffers);
  for (unsigned int i = 0; i < 2; i++) {
    glBindFramebuffer(GL_FRAMEBUFFER, pingpong_fbo[i]);
    glBindTexture(GL_TEXTURE_2D, pingpongBuffers[i]);
    glTexImage2D(
        GL_TEXTURE_2D, 0, GL_RGBA, 800, 540, 0, GL_RGBA, GL_FLOAT, NULL
    );
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glFramebufferTexture2D(
        GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, pingpongBuffers[i], 0
    );
  }

  // ------
  std::ifstream file(filename, std::ios::binary);

  std::vector< char > names;
  read_chunk(file, "str0", &names);

  struct HierarchyEntry {
    uint32_t parent;
    uint32_t name_begin;
    uint32_t name_end;
    glm::vec3 position;
    glm::quat rotation;
    glm::vec3 scale;
  };
  static_assert(sizeof(HierarchyEntry) == 4 + 4 + 4 + 4*3 + 4*4 + 4*3, "HierarchyEntry is packed.");
  std::vector< HierarchyEntry > hierarchy;
  read_chunk(file, "xfh0", &hierarchy);

  struct MeshEntry {
    uint32_t transform;
    uint32_t name_begin;
    uint32_t name_end;
  };
  static_assert(sizeof(MeshEntry) == 4 + 4 + 4, "MeshEntry is packed.");
  std::vector< MeshEntry > meshes;
  read_chunk(file, "msh0", &meshes);

  struct CameraEntry {
    uint32_t transform;
    char type[4]; //"pers" or "orth"
    float data; //fov in degrees for 'pers', scale for 'orth'
    float clip_near, clip_far;
  };
  static_assert(sizeof(CameraEntry) == 4 + 4 + 4 + 4 + 4, "CameraEntry is packed.");
  std::vector< CameraEntry > cameras;
  read_chunk(file, "cam0", &cameras);

  struct LightEntry {
    uint32_t transform;
    char type;
    glm::u8vec3 color;
    float energy;
    float distance;
    float fov;
  };
  static_assert(sizeof(LightEntry) == 4 + 1 + 3 + 4 + 4 + 4, "LightEntry is packed.");
  std::vector< LightEntry > lamps;
  read_chunk(file, "lmp0", &lamps);

  if (file.peek() != EOF) {
    std::cerr << "WARNING: trailing data in scene file '" << filename << "'" << std::endl;
  }

  //--------------------------------
  //Now that file is loaded, create transforms for hierarchy entries:

  std::vector< Transform * > hierarchy_transforms;
  hierarchy_transforms.reserve(hierarchy.size());

  for (auto const &h : hierarchy) {
    transforms.emplace_back();
    Transform *t = &transforms.back();
    if (h.parent != -1U) {
      if (h.parent >= hierarchy_transforms.size()) {
        throw std::runtime_error("scene file '" + filename + "' did not contain transforms in topological-sort order.");
      }
      t->parent = hierarchy_transforms[h.parent];
    }

    if (h.name_begin <= h.name_end && h.name_end <= names.size()) {
      t->name = std::string(names.begin() + h.name_begin, names.begin() + h.name_end);
    } else {
        throw std::runtime_error("scene file '" + filename + "' contains hierarchy entry with invalid name indices");
    }

    t->position = h.position;
    t->rotation = h.rotation;
    t->scale = h.scale;

    hierarchy_transforms.emplace_back(t);
  }
  assert(hierarchy_transforms.size() == hierarchy.size());

  for (auto const &m : meshes) {
    if (m.transform >= hierarchy_transforms.size()) {
      throw std::runtime_error("scene file '" + filename + "' contains mesh entry with invalid transform index (" + std::to_string(m.transform) + ")");
    }
    if (!(m.name_begin <= m.name_end && m.name_end <= names.size())) {
      throw std::runtime_error("scene file '" + filename + "' contains mesh entry with invalid name indices");
    }
    std::string name = std::string(names.begin() + m.name_begin, names.begin() + m.name_end);

    if (on_drawable) {
      on_drawable(*this, hierarchy_transforms[m.transform], name);
    }

  }

  for (auto const &c : cameras) {
    if (c.transform >= hierarchy_transforms.size()) {
      throw std::runtime_error("scene file '" + filename + "' contains camera entry with invalid transform index (" + std::to_string(c.transform) + ")");
    }
    if (std::string(c.type, 4) != "pers") {
      std::cout << "Ignoring non-perspective camera (" + std::string(c.type, 4) + ") stored in file." << std::endl;
      continue;
    }
    this->cameras.emplace_back(hierarchy_transforms[c.transform]);
    Camera *camera = &this->cameras.back();
    camera->fovy = c.data / 180.0f * 3.1415926f; //FOV is stored in degrees; convert to radians.
    camera->near = c.clip_near;
    //N.b. far plane is ignored because cameras use infinite perspective matrices.
  }

  for (auto const &l : lamps) {
    if (l.transform >= hierarchy_transforms.size()) {
      throw std::runtime_error("scene file '" + filename + "' contains lamp entry with invalid transform index (" + std::to_string(l.transform) + ")");
    }
    if (l.type == 'p') {
      //good
    } else if (l.type == 'h') {
      //fine
    } else if (l.type == 's') {
      //okay
    } else if (l.type == 'd') {
      //sure
    } else {
      std::cout << "Ignoring unrecognized lamp type (" + std::string(&l.type, 1) + ") stored in file." << std::endl;
      continue;
    }
    this->lamps.emplace_back(hierarchy_transforms[l.transform]);
    Lamp *lamp = &this->lamps.back();
    lamp->type = static_cast<Lamp::Type>(l.type);
    lamp->energy = glm::vec3(l.color) * l.energy;
    lamp->spot_fov = l.fov / 180.0f * 3.1415926f; //FOV is stored in degrees; convert to radians.
  }

}

//-------------------------
