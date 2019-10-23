#include "RollLevel.hpp"
#include "data_path.hpp"
#include "FirstpassProgram.hpp"
#include "PostprocessingProgram.hpp"
#include <glm/gtc/type_ptr.hpp>

#include <unordered_set>
#include <unordered_map>
#include <iostream>

// player:
Mesh const *mesh_player = nullptr;
// floor tiles
Mesh const *mesh_soil = nullptr;
Mesh const *mesh_path = nullptr;
Mesh const *mesh_unoccupied = nullptr;
// plant (carrot)
Mesh const *mesh_leaf1 = nullptr;
Mesh const *mesh_leaf2 = nullptr;
Mesh const *mesh_leaf3_root = nullptr;
Mesh const *mesh_leaf3_leaf = nullptr;
// trees
Mesh const *mesh_tree1 = nullptr;
Mesh const *mesh_tree2 = nullptr;
Mesh const *mesh_tree_trunk = nullptr;

//names of mesh-to-collider-mesh:
std::unordered_map< Mesh const *, Mesh const * > mesh_to_collider;

GLuint roll_meshes_for_firstpass_program = 0;

//Load the meshes used in Sphere Roll levels:
Load< MeshBuffer > roll_meshes(LoadTagDefault, []() -> MeshBuffer * {
  MeshBuffer *ret = new MeshBuffer(data_path("test_scene.pnct"));

  for (auto p : ret->meshes) {
    std::cout << p.first << std::endl;
  }

  //Build vertex array object for the program we're using to shade these meshes:
  roll_meshes_for_firstpass_program = ret->make_vao_for_program(firstpass_program->program);

  // player:
  mesh_player = &ret->lookup("player");
  // floor tiles
  mesh_soil = &ret->lookup("soil");
  mesh_path = &ret->lookup("path");
  mesh_unoccupied = &ret->lookup("unoccupied");
  // plant (carrot)
  mesh_leaf1 = &ret->lookup("leaf1");
  mesh_leaf2 = &ret->lookup("leaf2");
  mesh_leaf3_root = &ret->lookup("leaf3_root");
  mesh_leaf3_leaf = &ret->lookup("leaf3_leaf");
  // trees
  mesh_tree1 = &ret->lookup("tree_1");
  mesh_tree2 = &ret->lookup("tree_2");
  mesh_tree_trunk = &ret->lookup("tree_trunk");
  
  mesh_to_collider.insert(std::make_pair(&ret->lookup("player"), &ret->lookup("player")));
  mesh_to_collider.insert(std::make_pair(&ret->lookup("soil"), &ret->lookup("soil")));
  mesh_to_collider.insert(std::make_pair(&ret->lookup("path"), &ret->lookup("path")));
  mesh_to_collider.insert(std::make_pair(&ret->lookup("unoccupied"), &ret->lookup("unoccupied")));

  return ret;
});

//Load sphere roll level:
Load< RollLevel > game_scene(LoadTagLate, []() -> RollLevel* {
  RollLevel *ret = new RollLevel(data_path("test_scene.scene"));
  return ret;
});

//-------- RollLevel ---------

RollLevel::RollLevel(std::string const &scene_file) {

  srand48(time(NULL));

  //Load scene (using Scene::load function), building proper associations as needed:
  load(scene_file, [this,&scene_file](Scene &, Transform *transform, std::string const &mesh_name){
    Mesh const *mesh = &roll_meshes->lookup(mesh_name);
  
    drawables.emplace_back(transform);
    Drawable::Pipeline &pipeline = drawables.back().pipeline;
    glm::vec4 *custom_col = drawables.back().custom_col;
    assert(custom_col);
    
    //set up drawable to draw mesh from buffer:
    pipeline = firstpass_program_pipeline;
    pipeline.vao = roll_meshes_for_firstpass_program;
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
      if (f != mesh_to_collider.end()) {
      // assert (f != mesh_to_collider.end());
        mesh_colliders.emplace_back(transform, *f->second, *roll_meshes);
      }
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
