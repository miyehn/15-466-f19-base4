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
        throw std::runtime_error("Level '" + scene_file + "' contains more than one Sphere (starting location).");
      }
      player.transform = transform;
    } else if (mesh == mesh_window) {
      windows.emplace_back(transform, custom_col);
      assert(windows.back().custom_col);
      auto f = mesh_to_collider.find(mesh);
      mesh_colliders.emplace_back(transform, *f->second, *roll_meshes);
    } else if (mesh == mesh_letter) {
      letter.transform = transform;
      letter.default_rotation = transform->rotation;
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

  srand48(time(NULL));
  generate_letter();

}

float safe_drand() {
  float r = drand48();
  return r==1.0f ? 0.99999999f : r;
}

void RollLevel::generate_letter() {

  // set previous dest color to default
  if (letter.destination) {
    assert(letter.destination->custom_col);
    *letter.destination->custom_col = glm::vec4(1, 0, 1, 1);
  }
  // generate new dest
  letter.destination = &windows[(int)(safe_drand() * windows.size())];
  // get a random color from list
  glm::vec4 col = letter_colors[(int)(safe_drand() * letter_colors.size())];
  // set new dest color to custom
  assert(letter.destination && letter.destination->custom_col);
  *letter.destination->custom_col = glm::vec4(col.x, col.y, col.z, col.w);
  assert(letter.custom_col);
  *letter.custom_col = glm::vec4(col.x, col.y, col.z, col.w);

  letter.transform->position = glm::vec3(
      (drand48()-0.5f) * 80.0f, 
      (drand48()-0.5f) * 80.0f, 180);
}

void RollLevel::Letter::update_transform(Scene::Transform *plr_t, bool carrying, float elapsed) {
  if (carrying) {
    assert(plr_t);
    glm::vec3 offset = transform->rotation * glm::vec3(0, 0, 2);
    transform->position = plr_t->position + offset;
    transform->rotation = plr_t->rotation;
  } else {
    transform->rotation = default_rotation;
    float &z = transform->position.z;
    z += (140.0f - z) * elapsed * 0.25f;
  }
}
