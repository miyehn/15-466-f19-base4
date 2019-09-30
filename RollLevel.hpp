#pragma once

/*
 * A RollLevel is a scene augmented with some additional information which
 *   is useful when playing Sphere Roll.
 */

#include "Scene.hpp"
#include "Mesh.hpp"
#include "Load.hpp"

struct RollLevel;

//List of all levels:
extern Load< RollLevel > game_scene;

struct RollLevel : Scene {
  //Build from a scene:
  //  note: will throw on loading failure, or if certain critical objects don't appear
  RollLevel(std::string const &scene_file);

  //Solid parts of level are tracked as MeshColliders:
  struct MeshCollider {
    MeshCollider(Scene::Transform *transform_, Mesh const &mesh_, MeshBuffer const &buffer_) : transform(transform_), mesh(&mesh_), buffer(&buffer_) { }
    Scene::Transform *transform;
    Mesh const *mesh;
    MeshBuffer const *buffer;
  };

  std::vector<glm::vec4> letter_colors = {
    glm::vec4(1.0f, 0.5f, 0.5f, 1.0f),
    glm::vec4(0.5f, 1.0f, 0.5f, 1.0f),
    glm::vec4(0.5f, 0.5f, 1.0f, 1.0f)
  };

  struct Window {
    Window(Scene::Transform *transform_, glm::vec4* col_ptr) 
      : transform(transform_), custom_col(col_ptr) { };
    Scene::Transform *transform;
    bool isActive = false;
    glm::vec4 *custom_col;
  };
  
  struct Letter {
    void update_transform(Scene::Transform *plr_t, bool carrying, float elapsed);
    Scene::Transform *transform = nullptr;
    glm::quat default_rotation = glm::quat();
    glm::vec4 *custom_col = nullptr;
    Window *destination = nullptr;
  };

  //Sphere being rolled tracked using this structure:
  struct Player {
    Scene::Transform *transform = nullptr;
    glm::vec3 velocity = glm::vec3(0.0f, 0.0f, 0.0f);

    float view_azimuth = 0.0f;
    float elevation = 0.0f;

    float view_azimuth_acc = 0.0f;
    float elevation_acc = 0.0f;
  };

  void generate_letter();

  //Additional information for things in the level:
  Scene::Camera *camera = nullptr;
  std::vector< MeshCollider > mesh_colliders = {};
  std::vector< Window > windows = {};
  Letter letter;
  Player player;

  // other game states:
  bool carrying_letter = false;
  int delivery_count = 0;
  
};

