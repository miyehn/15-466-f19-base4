#include "RollLevel.hpp"
#include "data_path.hpp"
#include "LitColorTextureProgram.hpp"
#include "BloomProgram.hpp"
#include <glm/gtc/type_ptr.hpp>

#include <unordered_set>
#include <unordered_map>
#include <iostream>

//used for lookup later:
Mesh const *mesh_player = nullptr;
Mesh const *mesh_floor = nullptr;
Mesh const *mesh_blarge = nullptr;
Mesh const *mesh_bsmall = nullptr;

//names of mesh-to-collider-mesh:
std::unordered_map< Mesh const *, Mesh const * > mesh_to_collider;

GLuint roll_meshes_for_lit_color_texture_program = 0;

//Load the meshes used in Sphere Roll levels:
Load< MeshBuffer > roll_meshes(LoadTagDefault, []() -> MeshBuffer * {
  MeshBuffer *ret = new MeshBuffer(data_path("test_scene.pnct"));

  for (auto p : ret->meshes) {
    std::cout << p.first << std::endl;
  }

  //Build vertex array object for the program we're using to shade these meshes:
  roll_meshes_for_lit_color_texture_program = ret->make_vao_for_program(lit_color_texture_program->program);

  //key objects:
  mesh_player = &ret->lookup("player");
  mesh_floor = &ret->lookup("floor");
  mesh_blarge = &ret->lookup("blob_large");
  mesh_bsmall = &ret->lookup("blob_small");
  
  mesh_to_collider.insert(std::make_pair(&ret->lookup("player"), &ret->lookup("player")));
  mesh_to_collider.insert(std::make_pair(&ret->lookup("floor"), &ret->lookup("floor")));
  mesh_to_collider.insert(std::make_pair(&ret->lookup("blob_large"), &ret->lookup("blob_large")));
  mesh_to_collider.insert(std::make_pair(&ret->lookup("blob_small"), &ret->lookup("blob_small")));

  return ret;
});

//Load sphere roll level:
Load< RollLevel > game_scene(LoadTagLate, []() -> RollLevel* {
  RollLevel *ret = new RollLevel(data_path("test_scene.scene"));
  return ret;
});

Load< BloomProgram > bloom_program(LoadTagEarly, []() -> BloomProgram const * {
  BloomProgram *ret = new BloomProgram();
  return ret;
});

//-------- RollLevel ---------

RollLevel::RollLevel(std::string const &scene_file) {

  srand48(time(NULL));
  post_processing_program = bloom_program->program;

  //Load scene (using Scene::load function), building proper associations as needed:
  load(scene_file, [this,&scene_file](Scene &, Transform *transform, std::string const &mesh_name){
    Mesh const *mesh = &roll_meshes->lookup(mesh_name);
  
    drawables.emplace_back(transform);
    Drawable::Pipeline &pipeline = drawables.back().pipeline;
    glm::vec4 *custom_col = drawables.back().custom_col;
    assert(custom_col);
    
    //set up drawable to draw mesh from buffer:
    pipeline = lit_color_texture_program_pipeline;
    pipeline.vao = roll_meshes_for_lit_color_texture_program;
    pipeline.type = mesh->type;
    pipeline.start = mesh->start;
    pipeline.count = mesh->count;
    pipeline.set_uniforms = [&pipeline, custom_col, transform](){
      GLuint loc = glGetUniformLocation(pipeline.program, "CUSTOM_COL");
      assert(loc != -1U);
      assert (custom_col);
      glm::vec4 c = *custom_col;
      glUniform4f(loc, c.x, c.y, c.z, c.w);
      loc = glGetUniformLocation(pipeline.program, "ANCHOR_POS");
      assert(loc != -1U);
      glm::vec3 pos = transform->position;
      glUniform3f(loc, pos.x, pos.y, pos.z);
    };

    //associate level info with the drawable:
    if (mesh == mesh_player) {
      if (player.transform) {
        throw std::runtime_error("Level '" + scene_file + "' contains more than one player.");
      }
      player.transform = transform;
    } else {
      auto f = mesh_to_collider.find(mesh);
      assert (f != mesh_to_collider.end());
      mesh_colliders.emplace_back(transform, *f->second, *roll_meshes);
    }
  });

  if (!player.transform) {
    throw std::runtime_error("Level '" + scene_file + "' contains no player.");
  }

  std::cout << "Level '" << scene_file << "' has "
    << mesh_colliders.size() << " mesh colliders."
    << std::endl;
  
  //Create player camera:
  transforms.emplace_back();
  cameras.emplace_back(&transforms.back());
  camera = &cameras.back();

  camera->fovy = 60.0f / 180.0f * 3.1415926f;
  camera->near = 0.05f;

}
