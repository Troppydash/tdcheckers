#pragma once

#include "tdwindow.h"
#include "tdmesh.h"
#include "tdshader.h"
#include "tdtexture.h"

#include <tdcheckers/checkers.h>
#include <tdcheckers/explorer.h>
#include <glm/glm.hpp>

#include <mutex>
#include <queue>

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

		void restart_game();

		void finish_game();

	protected:
		// convert a viewport/screen coordinate to a world coordinate
		glm::vec2 screen_to_world(const glm::vec2 &screen) const;

		// make a move
		void perform_move();

		// highlight the piece and available moves for the selected piece
		void highlight_selection();

		void unhighlight_all();

		void handle_player(const std::pair<double, double> &clicked);

		void handle_computer();

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

		// squares highlighted & selected & captured
		uint64_t m_highlights;
		uint64_t m_selected;
		uint64_t m_captures;

	private:
		// representing the board input state
		enum class input_state
		{
			// viewing the board, no actions
			VIEW,
			// waiting for the computer, no actions
			COMPUTER,
			// waiting for a piece selection
			SELECTING,
			// waiting for a movement input
			MOVING,
		};

		// represent the board state
		struct board_state
		{
			checkers::state turn;
			input_state action;
			checkers::move partial_move;
		};
		
		board_state m_state;

		// represent an action queue of left clicks
		std::queue<std::pair<double, double>> m_action_clicks;

	private:  // eval part
		enum class board_type
		{
			PLAYER_PLAYER,
			PLAYER_AI,
			AI_AI
		};

		board_type m_type = board_type::PLAYER_AI;

		// evaluation hook
		std::thread m_eval;
		std::atomic<bool> m_thinking;
		bool m_eval_initialized = false;
	};
};