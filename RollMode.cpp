#include "RollMode.hpp"
#include "DrawLines.hpp"
#include "Mesh.hpp"
#include "Sprite.hpp"
#include "DrawSprites.hpp"
#include "data_path.hpp"
#include "Sound.hpp"
#include "collide.hpp"
#include "gl_errors.hpp"

//for glm::pow(quaternion, float):
#include <glm/gtx/quaternion.hpp>

#include <algorithm>
#include <iostream>

extern Load< RollLevel > game_scene;

Load< SpriteAtlas > trade_font_atlas(LoadTagDefault, []() -> SpriteAtlas const * {
  return new SpriteAtlas(data_path("trade-font"));
});

RollMode::RollMode(RollLevel level_) : level(level_)  {
  restart();
}

RollMode::~RollMode() {
}

bool RollMode::handle_event(SDL_Event const &evt, glm::uvec2 const &window_size) {

  if (evt.type == SDL_KEYDOWN || evt.type == SDL_KEYUP) {
    if (evt.key.keysym.scancode == SDL_SCANCODE_A) {
      controls.left = (evt.type == SDL_KEYDOWN);
      return true;
    } else if (evt.key.keysym.scancode == SDL_SCANCODE_D) {
      controls.right = (evt.type == SDL_KEYDOWN);
      return true;
    } else if (evt.key.keysym.scancode == SDL_SCANCODE_W) {
      controls.forward = (evt.type == SDL_KEYDOWN);
      return true;
    } else if (evt.key.keysym.scancode == SDL_SCANCODE_S) {
      controls.backward = (evt.type == SDL_KEYDOWN);
      return true;
    }
    if (evt.type==SDL_KEYDOWN && evt.key.keysym.sym==SDLK_SPACE) {
      display_text = !display_text;
    }
  }

  return false;
}

void RollMode::update(float elapsed) {

  //NOTE: turn this on to fly the sphere instead of rolling it -- makes collision debugging easier
  { //player motion:
    //build a shove from controls:

    //get references to player position/rotation because those are convenient to have:
    glm::vec3 &position = level.player.transform->position;
    glm::vec3 &velocity = level.player.velocity;
    glm::quat &rotation = level.player.transform->rotation;

    float t = elapsed / 40.0f;
    if (controls.left) level.player.view_azimuth_acc += t;
    if (controls.right) level.player.view_azimuth_acc -= t;
    if (controls.forward) level.player.elevation_acc -= t / 2.0f;
    if (controls.backward) level.player.elevation_acc += t / 2.0f;

    level.player.view_azimuth += level.player.view_azimuth_acc;
    level.player.elevation += level.player.elevation_acc;

    level.player.view_azimuth /= 2.0f * 3.1415926f;
    level.player.view_azimuth -= std::round(level.player.view_azimuth);
    level.player.view_azimuth *= 2.0f * 3.1415926f;

    level.player.elevation = std::max(-85.0f / 180.0f * 3.1415926f, level.player.elevation);
    level.player.elevation = std::min( 85.0f / 180.0f * 3.1415926f, level.player.elevation);

    rotation = 
        glm::angleAxis( level.player.view_azimuth, glm::vec3(0.0f, 0.0f, 1.0f) )
      * glm::angleAxis( level.player.elevation, glm::vec3(1.0f, 0.0f, 0.0f) );
    glm::vec3 shove = glm::mat3_cast(rotation) * glm::vec3(0.0f, 0.3f, 0.0f);
    shove *= 10.0f;

    //update player using shove:
    //decay existing velocity toward shove:
    velocity = glm::mix(shove, velocity, std::pow(0.5f, elapsed / 0.25f));

    // decay acc toward 0
    level.player.view_azimuth_acc -= level.player.view_azimuth_acc * elapsed * 2.0f;
    level.player.elevation_acc -= level.player.elevation_acc * elapsed * 2.0f;

    //collide against level:
    float remain = elapsed;
    
    for (int32_t iter = 0; iter < 10; ++iter) {
      if (remain == 0.0f) break;

      float sphere_radius = 1.0f; //player sphere is radius-1
      glm::vec3 sphere_sweep_from = position;
      glm::vec3 sphere_sweep_to = position + velocity * remain;

      glm::vec3 sphere_sweep_min = glm::min(sphere_sweep_from, sphere_sweep_to) - glm::vec3(sphere_radius);
      glm::vec3 sphere_sweep_max = glm::max(sphere_sweep_from, sphere_sweep_to) + glm::vec3(sphere_radius);

      bool collided = false;
      float collision_t = 1.0f;
      glm::vec3 collision_at = glm::vec3(0.0f);
      glm::vec3 collision_out = glm::vec3(0.0f);
      for (auto const &collider : level.mesh_colliders) {
        glm::mat4x3 collider_to_world = collider.transform->make_local_to_world();

        { //Early discard:
          // check if AABB of collider overlaps AABB of swept sphere:

          //(1) compute bounding box of collider in world space:
          glm::vec3 local_center = 0.5f * (collider.mesh->max + collider.mesh->min);
          glm::vec3 local_radius = 0.5f * (collider.mesh->max - collider.mesh->min);

          glm::vec3 world_min = collider_to_world * glm::vec4(local_center, 1.0f)
            - glm::abs(local_radius.x * collider_to_world[0])
            - glm::abs(local_radius.y * collider_to_world[1])
            - glm::abs(local_radius.z * collider_to_world[2]);
          glm::vec3 world_max = collider_to_world * glm::vec4(local_center, 1.0f)
            + glm::abs(local_radius.x * collider_to_world[0])
            + glm::abs(local_radius.y * collider_to_world[1])
            + glm::abs(local_radius.z * collider_to_world[2]);

          //(2) check if bounding boxes overlap:
          bool can_skip = !collide_AABB_vs_AABB(sphere_sweep_min, sphere_sweep_max, world_min, world_max);

          if (can_skip) {
            //AABBs do not overlap; skip detailed check:
            continue;
          }
        }

        //Full (all triangles) test:
        assert(collider.mesh->type == GL_TRIANGLES); //only have code for TRIANGLES not other primitive types
        for (GLuint v = 0; v + 2 < collider.mesh->count; v += 3) {
          //get vertex positions from associated positions buffer:
          //  (and transform to world space)
          glm::vec3 a = collider_to_world * glm::vec4(collider.buffer->positions[collider.mesh->start+v+0], 1.0f);
          glm::vec3 b = collider_to_world * glm::vec4(collider.buffer->positions[collider.mesh->start+v+1], 1.0f);
          glm::vec3 c = collider_to_world * glm::vec4(collider.buffer->positions[collider.mesh->start+v+2], 1.0f);
          //check triangle:
          bool did_collide = collide_swept_sphere_vs_triangle(
            sphere_sweep_from, sphere_sweep_to, sphere_radius,
            a,b,c,
            &collision_t, &collision_at, &collision_out);

          if (did_collide) {
            collided = true;
            if (collider.transform == level.letter.destination->transform) {
              if (level.carrying_letter) {
                level.carrying_letter = false;
                level.delivery_count++;
                level.generate_letter();
              }
            }
          }

        }
      }

      if (!collided) {
        position = sphere_sweep_to;
        remain = 0.0f;
        break;
      } else {
        position = glm::mix(sphere_sweep_from, sphere_sweep_to, collision_t);
        float d = glm::dot(velocity, collision_out);
        if (d < 0.0f) {
          velocity -= (1.1f * d) * collision_out;
        }
        remain = (1.0f - collision_t) * remain;
      }
    }
  }
  
  // update letter location
  level.letter.update_transform(level.player.transform, level.carrying_letter, elapsed);

  // check if player hits letter
  if (!level.carrying_letter && 
      // ok I know I'm assuming they're both in world coordinates... In this case they are
      glm::length(level.letter.transform->position - level.player.transform->position) < 1.5f) {
    level.carrying_letter = true;
  }

  { //camera update:

    glm::quat plr_rotation = level.player.transform->rotation;
    glm::vec3 plr_position = level.player.transform->position;

    glm::quat target_rotation = plr_rotation * glm::angleAxis(0.35f * 3.1415926525f, glm::vec3(1.0f, 0.0f, 0.0f));
    glm::vec3 target_position = plr_position + 
      glm::mat3_cast(plr_rotation) * glm::vec3(0.0f, -16.0f, 8.0f);

    glm::quat &cam_rotation = level.camera->transform->rotation;
    glm::vec3 &cam_position = level.camera->transform->position;

    glm::vec3 &position = cam_position;
    glm::vec3 velocity = (target_position - cam_position) * 2.8f;

    //collide against level:
    float remain = elapsed;
    
    for (int32_t iter = 0; iter < 10; ++iter) {
      if (remain == 0.0f) break;

      float sphere_radius = 3.0f; // don't allow camera to get to, say, 3 units from any obstacle
      glm::vec3 sphere_sweep_from = position;
      glm::vec3 sphere_sweep_to = position + velocity * remain;

      glm::vec3 sphere_sweep_min = glm::min(sphere_sweep_from, sphere_sweep_to) - glm::vec3(sphere_radius);
      glm::vec3 sphere_sweep_max = glm::max(sphere_sweep_from, sphere_sweep_to) + glm::vec3(sphere_radius);

      bool collided = false;
      float collision_t = 1.0f;
      glm::vec3 collision_at = glm::vec3(0.0f);
      glm::vec3 collision_out = glm::vec3(0.0f);
      for (auto const &collider : level.mesh_colliders) {
        glm::mat4x3 collider_to_world = collider.transform->make_local_to_world();

        { //Early discard:
          // check if AABB of collider overlaps AABB of swept sphere:

          //(1) compute bounding box of collider in world space:
          glm::vec3 local_center = 0.5f * (collider.mesh->max + collider.mesh->min);
          glm::vec3 local_radius = 0.5f * (collider.mesh->max - collider.mesh->min);

          glm::vec3 world_min = collider_to_world * glm::vec4(local_center, 1.0f)
            - glm::abs(local_radius.x * collider_to_world[0])
            - glm::abs(local_radius.y * collider_to_world[1])
            - glm::abs(local_radius.z * collider_to_world[2]);
          glm::vec3 world_max = collider_to_world * glm::vec4(local_center, 1.0f)
            + glm::abs(local_radius.x * collider_to_world[0])
            + glm::abs(local_radius.y * collider_to_world[1])
            + glm::abs(local_radius.z * collider_to_world[2]);

          //(2) check if bounding boxes overlap:
          bool can_skip = !collide_AABB_vs_AABB(sphere_sweep_min, sphere_sweep_max, world_min, world_max);

          if (can_skip) {
            //AABBs do not overlap; skip detailed check:
            continue;
          }
        }

        //Full (all triangles) test:
        assert(collider.mesh->type == GL_TRIANGLES); //only have code for TRIANGLES not other primitive types
        for (GLuint v = 0; v + 2 < collider.mesh->count; v += 3) {
          //get vertex positions from associated positions buffer:
          //  (and transform to world space)
          glm::vec3 a = collider_to_world * glm::vec4(collider.buffer->positions[collider.mesh->start+v+0], 1.0f);
          glm::vec3 b = collider_to_world * glm::vec4(collider.buffer->positions[collider.mesh->start+v+1], 1.0f);
          glm::vec3 c = collider_to_world * glm::vec4(collider.buffer->positions[collider.mesh->start+v+2], 1.0f);
          //check triangle:
          bool did_collide = collide_swept_sphere_vs_triangle(
            sphere_sweep_from, sphere_sweep_to, sphere_radius,
            a,b,c,
            &collision_t, &collision_at, &collision_out);

          if (did_collide) {
            collided = true;
          }

        }
      }

      if (!collided) {
        cam_position = sphere_sweep_to;
        remain = 0.0f;
        break;
      } else {
        position = glm::mix(sphere_sweep_from, sphere_sweep_to, collision_t);
        float d = glm::dot(velocity, collision_out);
        if (d < 0.0f) {
          velocity -= (1.01f * d) * collision_out;
        }
        remain = (1.0f - collision_t) * remain;
      }
    }
    
    cam_rotation = glm::slerp(cam_rotation, target_rotation, 2.0f * elapsed);
  }
}

void RollMode::draw(glm::uvec2 const &drawable_size) {
  //--- actual drawing ---
  glClearColor(0.45f, 0.45f, 0.50f, 0.0f);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  glDisable(GL_BLEND);
  glEnable(GL_DEPTH_TEST);
  glDepthFunc(GL_LEQUAL);

  level.camera->aspect = drawable_size.x / float(drawable_size.y);
  level.draw(drawable_size, *level.camera);

  if (display_text) { //help text overlay:
    glDisable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendEquation(GL_FUNC_ADD);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    DrawSprites draw(*trade_font_atlas, glm::vec2(0,0), glm::vec2(320, 200), drawable_size, DrawSprites::AlignPixelPerfect);

    {
      std::string help_text = "packages delivered: " + std::to_string(level.delivery_count);
      glm::vec2 min, max;
      draw.get_text_extents(help_text, glm::vec2(0.0f, 0.0f), 1.0f, &min, &max);
      float x = std::round(30.0f - (0.3f * (max.x + min.x)));
      draw.draw_text(help_text, glm::vec2(x, 2.0f), 1.0f, glm::u8vec4(0xff,0xff,0xff,0xff));
    }
  }

  GL_ERRORS();
}

void RollMode::restart() {
  // level = start;

  glm::quat plr_rotation = level.player.transform->rotation;
  glm::vec3 plr_position = level.player.transform->position;

  glm::quat target_rotation = plr_rotation * glm::angleAxis(0.35f * 3.1415926525f, glm::vec3(1.0f, 0.0f, 0.0f));
  glm::vec3 target_position = plr_position + 
    glm::mat3_cast(plr_rotation) * glm::vec3(0.0f, -16.0f, 8.0f);

  level.camera->transform->rotation = target_rotation;
  level.camera->transform->position = target_position;
  
}
