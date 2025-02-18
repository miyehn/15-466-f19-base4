#pragma once

#include "Mode.hpp"
#include "RollLevel.hpp"
#include "DrawLines.hpp"

#include <memory>

struct RollMode : Mode {
	RollMode(RollLevel level_);
	virtual ~RollMode();

	virtual bool handle_event(SDL_Event const &, glm::uvec2 const &window_size) override;
	virtual void update(float elapsed) override;
	virtual void draw(glm::uvec2 const &drawable_size) override;

	//The (active, being-played) level:
	void restart();
	RollLevel level;
  bool display_text = true;

	//Current control signals:
	struct {
		bool forward = false;
		bool backward = false;
		bool left = false;
		bool right = false;
	} controls;

	//some debug drawing done during update:
	std::unique_ptr< DrawLines > DEBUG_draw_lines;
};
