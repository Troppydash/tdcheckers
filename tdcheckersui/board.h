#pragma once

#include "tdwindow.h"
#include "tdmesh.h"
#include "tdshader.h"
#include "tdtexture.h"

#include <tdcheckers/checkers.h>
#include <tdcheckers/explorer.h>
#include <glm/glm.hpp>

#include <mutex>

namespace gui
{
	class checkers_board : public td::window_object
	{
	public:
		checkers_board();

		void start(td::window &window) override;

		void update(float dt) override;

		void render() override;

		void end() override;

		glm::vec2 screen_to_world(const glm::vec2 &screen) const;

	private:
		// shaders
		td::graphics_shader m_boardshader;
		td::graphics_shader m_highlightshader;
		td::graphics_shader m_pieceshader;

		// mesh
		td::mesh m_squaremesh;

		// for positioning the board
		glm::mat4 m_matpersp;
		glm::mat4 m_matworld;

		// window reference
		td::window *m_window = nullptr;
	protected:
		// board stuff
		checkers::board m_board;
		std::mutex m_boardmutex;

		// squares highlighted & selected
		uint64_t m_highlights;
		uint64_t m_selected;

		// TODO: player input system




		// evaluation hook
		std::thread m_eval;
		bool m_gamestarted = false;
	};
};