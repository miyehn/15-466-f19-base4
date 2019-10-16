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

  //Sphere being rolled tracked using this structure:
  struct Player {
    Scene::Transform *transform = nullptr;
    glm::vec3 velocity = glm::vec3(0.0f, 0.0f, 0.0f);

    float view_azimuth = 0.0f;
    float elevation = 0.0f;

    float view_azimuth_acc = 0.0f;
    float elevation_acc = 0.0f;
  };

  //Additional information for things in the level:
  Scene::Camera *camera = nullptr;
  std::vector< MeshCollider > mesh_colliders = {};
  Player player;
  
};

