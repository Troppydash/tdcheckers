#include "board.h"

#include <vector>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <functional>
#include <thread>

// map a function for all non-zero bits on the checker board
static void map_bits(uint64_t bits, std::function<void(int, int, uint64_t)> &&fn)
{
	uint64_t mask = 1ull;
	for (int row = G_CHECKERS_WIDTH - 1; row >= 0; --row)
	{
		for (int col = 0; col < G_CHECKERS_WIDTH; ++col)
		{

			if ((mask & bits) > 0)
			{
				fn(row, col, mask);
			}

			mask <<= 1;
		}
	}
}

/**
 * @brief Converts a row and column index into a model transform matrix.
 *
 * The model must be anchored (0, 0) on the bottom left.
 *
 * @param row
 * @param col
 * @return
*/
static glm::mat4 make_cell_mat(int row, int col)
{
	// move the box to the place
	float scale = 1.0f / G_CHECKERS_WIDTH;
	glm::mat4 world(1.0f);

	// -0.5 + col * scale + 0.5 * scale
	// board + col disp + centering offset
	world = glm::translate(world, { col * scale - 0.5f + 0.5f * scale, row * scale - 0.5f + 0.5f * scale, 0.0f });
	world = glm::scale(world, { scale, scale, 1.0f });
	return world;
}

/**
 * @brief Converts a centered world coordinate to cell coordinates.
 *
 * @param x
 * @param y
 * @return
*/
static int make_cell_coord(double x, double y)
{
	int xcoord = round((x + 0.5) * G_CHECKERS_WIDTH - 0.5);
	int ycoord = (G_CHECKERS_WIDTH - 1) - round((y + 0.5) * G_CHECKERS_WIDTH - 0.5);

	return xcoord + ycoord * G_CHECKERS_WIDTH;
}

static bool valid_cell(int cell)
{
	if (cell < 0)
		return false;

	if (cell >= G_CHECKERS_SIZE)
		return false;

	return true;
}


gui::checkers_board::checkers_board()
	: m_highlights(0ull), m_selected(0ull), m_matpersp(1.0f), m_matworld(1.0f)
{
}

void gui::checkers_board::start(td::window &window)
{
	// mesh vertices, VBO and EBO
	std::vector<float> square{
		-0.5f, 0.5f, 0.0f, 0.0f, 1.0f,
			0.5f, 0.5f, 0.0f, 1.0f, 1.0f,
			0.5f, -0.5f, 0.0f, 1.0f, 0.0f,
			-0.5f, -0.5f, 0.0f, 0.0f, 0.0f
	};
	std::vector<unsigned int> square_indices{
		0, 1, 2,
			0, 2, 3
	};



	// load shaders
	m_boardshader.load("resources/shaders/board/vert.glsl", "resources/shaders/board/frag.glsl");
	m_highlightshader.load("resources/shaders/highlight/vert.glsl", "resources/shaders/highlight/frag.glsl");
	m_pieceshader.load("resources/shaders/piece/vert.glsl", "resources/shaders/piece/frag.glsl");


	// load square mesh
	m_squaremesh.load5(square);
	m_squaremesh.load_indices(square_indices);


	// load matrices
	// orthonormal projection at left=-width/2, right=+width/2, top=0.5, bottom=-0.5
	float relwidth = static_cast<float>(window.get_settings().width) / window.get_settings().height;
	m_matpersp = glm::ortho(-relwidth / 2, relwidth / 2, -0.5f, 0.5f);

	// world = I
	m_matworld = glm::mat4(1.0f);

	m_boardshader.set_uniform_matrix("perspective", glm::value_ptr(m_matpersp));
	m_boardshader.set_uniform_matrix("world", glm::value_ptr(m_matworld));

	m_highlightshader.set_uniform_matrix("perspective", glm::value_ptr(m_matpersp));
	m_highlightshader.set_uniform_matrix("world", glm::value_ptr(m_matworld));

	m_pieceshader.set_uniform_matrix("perspective", glm::value_ptr(m_matpersp));
	m_pieceshader.set_uniform_matrix("world", glm::value_ptr(m_matworld));

	//m_board.reset();
	const char initial[] =
		". . . O . . . x"
		"x . x . . . . ."
		". x . x . . . x"
		"o . x . . . x ."
		". o . . . o . o"
		"o . . . O . o ."
		". o . . . . . ."
		". . . . o . o .";
	std::string position(initial);
	m_board = checkers::board(position);
	m_window = &window;

	// register callbacks
	m_window->m_inputs.cursor_clicked.observe([&](const auto &a, const auto &b)
	{
		m_action_clicks.push(b);
	});
}

void gui::checkers_board::update(float dt)
{
	// fetch action
	if (m_action_clicks.empty())
		return;

	std::pair<double, double> clicked = m_action_clicks.front();
	m_action_clicks.pop();

	// state machine
	switch (m_state.action)
	{
	case input_state::SELECTING:
	{
		glm::vec2 location = screen_to_world({ clicked.first, clicked.second });
		int cell = make_cell_coord(location.x, location.y);

		// discard errornous inputs
		if (!valid_cell(cell))
			break;

		uint64_t mask = 1ull << cell;
		uint64_t piece_mask = m_board.get_player(m_state.turn);

		// return if not clicking on your piece
		if ((mask & piece_mask) == 0)
			break;

		// enter selection phase
		m_selected = mask;

		m_state.partial_move = checkers::move();
		m_state.partial_move.from = mask;
		if (mask & m_board.get_kings(m_state.turn))
			m_state.partial_move.king = true;

		m_state.action = input_state::MOVING;

		// display possible moves
		highlight_selection();

		break;
	}

	case input_state::MOVING:
	{
		glm::vec2 location = screen_to_world({ clicked.first, clicked.second });
		int cell = make_cell_coord(location.x, location.y);

		// discard errornous inputs
		if (!valid_cell(cell))
			break;

		uint64_t mask = 1ull << cell;

		// if clicking on last move to confirm
		if (mask == m_state.partial_move.to)
		{
			perform_move();
			unhighlight_all();
			m_state.action = input_state::SELECTING;
			break;
		}

		// if outside of the selectable squares, return
		if ((mask & m_highlights) == 0)
		{
			unhighlight_all();
			m_state.action = input_state::SELECTING;
			break;
		}

		m_state.partial_move.add_landing(mask);
		std::cout << m_state.partial_move.str() << std::endl;

		// check if selecting a valid location (also multiple)
		auto moves = m_board.compute_moves(m_state.turn);
		std::vector<checkers::move> valid_moves;
		for (auto &move : moves)
		{
			if (!move.subset_equal(m_state.partial_move))
				continue;

			valid_moves.push_back(move);
		}

		if (valid_moves.size() == 1)
		{
			perform_move();
			unhighlight_all();
			m_state.action = input_state::SELECTING;
			break;
		}

		highlight_selection();
		break;
	}
	}


	/*if (m_gamestarted)
		return;

	m_gamestarted = true;*/


	// engines
	//std::thread eval([&]()
	//{
	//	checkers::state turn = checkers::state::RED;
	//	explorer::optimizer optimizer_red(m_board, checkers::state::RED);
	//	explorer::optimizer optimizer_black(m_board, checkers::state::BLACK);

	//	while (true)
	//	{
	//		if (m_board.get_state(turn) != checkers::state::NONE)
	//		{
	//			return;
	//		}

	//		// compute move
	//		std::optional<checkers::move> best;
	//		if (turn == checkers::state::RED)
	//		{
	//			optimizer_red.update_board(m_board);
	//			optimizer_red.compute_score(turn, true);
	//			best = optimizer_red.get_move();
	//		}
	//		else
	//		{
	//			optimizer_black.update_board(m_board);
	//			optimizer_black.compute_score(turn, true);
	//			best = optimizer_black.get_move();
	//		}
	//
	//		m_boardmutex.lock();
	//		m_board = m_board.perform_move(best.value(), turn);
	//		m_highlights = best.value().from ^ best.value().to;
	//		m_boardmutex.unlock();

	//		turn = checkers::state_flip(turn);
	//	}
	//});

	//m_eval = std::move(eval);
}

void gui::checkers_board::render()
{
	m_boardshader.use();
	m_squaremesh.render();

	// draw highlights
	m_boardmutex.lock();
	uint64_t highlights = m_highlights;
	uint64_t selected = m_selected;
	uint64_t captures = m_captures;
	uint64_t redpieces = m_board.get_player(checkers::state::RED);
	uint64_t blackpieces = m_board.get_player(checkers::state::BLACK);
	uint64_t kings = m_board.get_kings(checkers::state::RED) | m_board.get_kings(checkers::state::BLACK);
	m_boardmutex.unlock();

	m_highlightshader.set_uniform_vector("highlight", { 1.0f, 1.0f, 0.0f });
	map_bits(highlights, [&](int row, int col, uint64_t mask)
	{
		// move the box to the place
		glm::mat4 world = make_cell_mat(row, col);

		m_highlightshader.set_uniform_matrix("world", glm::value_ptr(world));

		m_highlightshader.use();
		m_squaremesh.render();
	});

	m_highlightshader.set_uniform_vector("highlight", { 0.0f, 1.0f, 0.0f });
	map_bits(selected, [&](int row, int col, uint64_t mask)
	{
		// move the box to the place
		glm::mat4 world = make_cell_mat(row, col);

		m_highlightshader.set_uniform_matrix("world", glm::value_ptr(world));

		m_highlightshader.use();
		m_squaremesh.render();
	});


	m_highlightshader.set_uniform_vector("highlight", { 1.0f, 0.0f, 0.0f });
	map_bits(captures, [&](int row, int col, uint64_t mask)
	{
		// move the box to the place
		glm::mat4 world = make_cell_mat(row, col);

		m_highlightshader.set_uniform_matrix("world", glm::value_ptr(world));

		m_highlightshader.use();
		m_squaremesh.render();
	});



	// draw red pieces
	m_pieceshader.set_uniform_scalar("state", 0.0f);
	map_bits(redpieces, [&](int row, int col, uint64_t mask)
	{
		if (mask & kings)
			m_pieceshader.set_uniform_scalar("king", 1.0f);
		else
			m_pieceshader.set_uniform_scalar("king", 0.0f);

		glm::mat4 world = make_cell_mat(row, col);
		m_pieceshader.set_uniform_matrix("world", glm::value_ptr(world));
		m_pieceshader.use();
		m_squaremesh.render();
	});


	// draw black pieces
	m_pieceshader.set_uniform_scalar("state", 1.0f);
	map_bits(blackpieces, [&](int row, int col, uint64_t mask)
	{
		if (mask & kings)
			m_pieceshader.set_uniform_scalar("king", 1.0f);
		else
			m_pieceshader.set_uniform_scalar("king", 0.0f);

		glm::mat4 world = make_cell_mat(row, col);
		m_pieceshader.set_uniform_matrix("world", glm::value_ptr(world));
		m_pieceshader.use();
		m_squaremesh.render();
	});
}

void gui::checkers_board::end()
{
	m_boardshader.end();
	m_highlightshader.end();
	m_pieceshader.end();

	m_squaremesh.end();
}

void gui::checkers_board::restart_game()
{
	m_highlights = 0ull;
	m_selected = 0ull;
	m_state = { checkers::state::RED, input_state::SELECTING };
	m_board.reset();
}

void gui::checkers_board::finish_game()
{
	m_state = { checkers::state::NONE, input_state::VIEW };
}

glm::vec2 gui::checkers_board::screen_to_world(const glm::vec2 &screen) const
{
	double vwidth = m_window->get_settings().width;
	double vheight = m_window->get_settings().height;

	// apply the inverse viewport transform, note the inverse on the screen.y coordinates
	glm::vec2 trans{
		2.0 * screen.x / vwidth - 1.0f,
			-(2.0 * screen.y / vheight - 1.0f)
	};

	// invert perspective matrix
	glm::vec4 world = glm::inverse(m_matpersp) * glm::vec4{ trans.x, trans.y, 0.0f, 1.0f };

	return { world.x, world.y };
}

void gui::checkers_board::perform_move()
{
	m_board = m_board.perform_move(m_state.partial_move, m_state.turn);

	m_state.turn = checkers::state_flip(m_state.turn);
	m_state.partial_move = checkers::move();
}

void gui::checkers_board::highlight_selection()
{
	// highlight selection
	m_selected = m_state.partial_move.from;
	if (m_state.partial_move.captures.size() > 0)
		m_selected = m_state.partial_move.to;
	m_captures = 0ull;
	m_highlights = 0ull;


	// compute available moves
	auto moves = m_board.compute_moves(m_state.turn);
	for (auto &move : moves)
	{
		// not a possible move
		if (!move.subset_equal(m_state.partial_move))
		{
			continue;
		}

		// highlight captures
		for (auto capture : move.captures)
		{
			m_captures |= capture;
		}

		// highlight landings
		auto landings = m_state.partial_move.landings();
		if (landings.size() > 0)
		{
			m_highlights |= move.landings()[landings.size() - 1];
		}

		if (move.landings().size() > landings.size())
		{
			m_highlights |= move.landings()[landings.size()];
		}
	}
}

void gui::checkers_board::unhighlight_all()
{
	m_highlights = 0ull;
	m_captures = 0ull;
	m_selected = 0ull;
}
