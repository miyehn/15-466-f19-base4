#include "RollLevel.hpp"
#include "data_path.hpp"
#include "LitColorTextureProgram.hpp"
#include <glm/gtc/type_ptr.hpp>

#include <unordered_set>
#include <unordered_map>
#include <iostream>

//used for lookup later:
Mesh const *mesh_window = nullptr;
Mesh const *mesh_letter = nullptr;
Mesh const *mesh_player = nullptr;

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
  mesh_window = &ret->lookup("window");
  mesh_letter = &ret->lookup("letter");
  mesh_player = &ret->lookup("player");
  
  mesh_to_collider.insert(std::make_pair(&ret->lookup("obstacles"), &ret->lookup("obstacles")));
  mesh_to_collider.insert(std::make_pair(&ret->lookup("window"), &ret->lookup("window")));

  return ret;
});

//Load sphere roll level:
Load< RollLevel > game_scene(LoadTagLate, []() -> RollLevel* {
  RollLevel *ret = new RollLevel(data_path("test_scene.scene"));
  return ret;
});

//-------- RollLevel ---------

RollLevel::RollLevel(std::string const &scene_file) {

  //Load scene (using Scene::load function), building proper associations as needed:
  load(scene_file, [this,&scene_file](Scene &, Transform *transform, std::string const &mesh_name){
    Mesh const *mesh = &roll_meshes->lookup(mesh_name);
  
    drawables.emplace_back(transform);
    Drawable::Pipeline &pipeline = drawables.back().pipeline;
    glm::vec4 *custom_col = drawables.back().custom_col;
    
    //set up drawable to draw mesh from buffer:
    pipeline = lit_color_texture_program_pipeline;
    pipeline.vao = roll_meshes_for_lit_color_texture_program;
    pipeline.type = mesh->type;
    pipeline.start = mesh->start;
    pipeline.count = mesh->count;
    pipeline.set_uniforms = [&pipeline, custom_col](){
      GLuint loc = glGetUniformLocation(pipeline.program, "CUSTOM_COL");
      assert(loc != -1U);
      assert (custom_col);
      glm::vec4 c = *custom_col;
      glUniform4f(loc, c.x, c.y, c.z, c.w);
    };

    //associate level info with the drawable:
    if (mesh == mesh_player) {
      if (player.transform) {
        throw std::runtime_error("Level '" + scene_file + "' contains more than one Sphere (starting location).");
      }
      player.transform = transform;
    } else if (mesh == mesh_window) {
      windows.emplace_back(transform, custom_col);
      auto f = mesh_to_collider.find(mesh);
      mesh_colliders.emplace_back(transform, *f->second, *roll_meshes);
    } else if (mesh == mesh_letter) {
      letter.transform = transform;
      letter.custom_col = custom_col;
    } else {
      auto f = mesh_to_collider.find(mesh);
      assert (f != mesh_to_collider.end());
      mesh_colliders.emplace_back(transform, *f->second, *roll_meshes);
    }
  });

  if (!player.transform) {
    throw std::runtime_error("Level '" + scene_file + "' contains no Sphere (starting location).");
  }

  std::cout << "Level '" << scene_file << "' has "
    << mesh_colliders.size() << " mesh colliders, "
    << windows.size() << " windows "
    << std::endl;
  
  //Create player camera:
  transforms.emplace_back();
  cameras.emplace_back(&transforms.back());
  camera = &cameras.back();

  camera->fovy = 60.0f / 180.0f * 3.1415926f;
  camera->near = 0.05f;

  generate_letter();

}

void RollLevel::generate_letter() {
  // set previous dest color to default
  if (letter.destination) {
    delete letter.destination->custom_col;
    letter.destination->custom_col = new glm::vec4(1, 0, 0, 1);
  }
  // generate new dest
  letter.destination = &windows[0];
  // set new dest color to custom
  if (letter.destination) {
    delete letter.destination->custom_col;
    letter.destination->custom_col = new glm::vec4(0.2, 0.8, 0.8, 1);
  }
  if (letter.custom_col) delete (letter.custom_col);
  letter.custom_col = new glm::vec4(0.2, 0.8, 0.8, 1);

  letter.transform->position = player.transform->position;
}

void RollLevel::Letter::update_location(bool carrying) {

}
