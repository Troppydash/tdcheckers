#include "explorer.h"

explorer::optimizer::optimizer(checkers::board board, checkers::state turn)
	: m_board(board), m_player(turn)
{
}

struct evaluate_extra
{
	std::unordered_map<uint64_t, std::pair<int, float>> transposition;
	size_t exploration;
};


template <typename T> int sgn(T val) {
	return (T(0) < val) - (val < T(0));
}

static float evaluate(
	checkers::board board,
	checkers::state turn,
	checkers::state player,
	int depth_remaining,
	float alpha, float beta,
	bool maxing,
	// extra
	evaluate_extra &extra
)
{
	extra.exploration += 1;

	// use transposition
	uint64_t hash = checkers::board::hash_function()(board) ^ std::hash<bool>()(turn == player);

	if (extra.transposition.count(hash) != 0)
	{
		auto &data = extra.transposition[hash];

		// only return the transposition if the stored depth is higher than remaining
		if (data.first >= depth_remaining)
			return data.second;
	}

	std::vector<checkers::move> moves = board.compute_moves(turn);
	if (moves.size() == 0)
	{
		if (turn == player)
		{
			extra.transposition[hash] = { 100, -100 };
			return -100;
		}
		else
		{
			extra.transposition[hash] = { 100, 100 };
			return 100;
		}
	}

	if (depth_remaining == 0)
	{
		// assuming draw
		return 0;
	}


	checkers::state nextturn = (turn == checkers::state::RED) ? checkers::state::BLACK : checkers::state::RED;


	float value;
	if (maxing)
	{
		value = -1e9;

		for (auto &move : moves)
		{
			float newvalue = evaluate(
				board.perform_move(move, turn),
				nextturn,
				player,
				depth_remaining - 1,
				alpha,
				beta,
				false,
				extra
			);
			newvalue -= sgn(newvalue) * 0.1f;

			if (newvalue > value)
				value = newvalue;

			if (value > beta)
				break;

			if (value > alpha)
				alpha = value;
		}
	}
	else
	{
		value = 1e9;

		for (auto &move : moves)
		{
			float newvalue = evaluate(
				board.perform_move(move, turn),
				nextturn,
				player,
				depth_remaining - 1,
				alpha,
				beta,
				true,
				extra
			);
			newvalue -= sgn(newvalue) * 0.1f;

			if (newvalue < value)
				value = newvalue;

			if (value < alpha)
				break;

			if (value < beta)
				beta = value;
		}
	}

	// update transposition
	if (extra.transposition.count(hash) != 0)
	{
		auto &data = extra.transposition[hash];
		if (data.first < depth_remaining)
			extra.transposition[hash] = { depth_remaining, value };
	}
	else
	{
		extra.transposition[hash] = { depth_remaining, value };
	}

	return value;
}

void explorer::optimizer::compute_score(checkers::state turn)
{
	// scores follow the definition
	// + for red winning
	// - for black winning
	// the higher the |score|, the larger the advantage
	// 0 is even

	evaluate_extra extra{
		{},
		0
	};

	m_score = 0;

	// iterative deepining
	int startdepth = 5;
	int enddepth = 15;
	for (int depth = startdepth; depth < enddepth; ++depth)
	{
		m_score = evaluate(
			m_board,
			turn,
			m_player,
			depth,
			-1e9,
			1e9,
			m_player == turn,
			extra
		);
		std::cout << "At depth " << depth << ", score = " << m_score << std::endl;

		// exit when the score is sure
		if (abs(m_score) > 50)
			break;
	}


	// save table
	m_transposition = extra.transposition;

	// compute best move
	auto hashing = checkers::board::hash_function();

	// min-max state
	bool maxing = turn == m_player;


	// else get the moves
	auto moves = m_board.compute_moves(turn);
	checkers::state nextturn = checkers::state_flip(turn);

	// min/max
	int best = -1;
	if (maxing)
	{
		float score = -1e9;
		for (int j = 0; j < moves.size(); ++j)
		{
			uint64_t hash = hashing(m_board.perform_move(moves[j], turn)) ^ std::hash<bool>()(false);
			if (extra.transposition.count(hash) == 0)
				continue;

			auto &data = extra.transposition[hash];
			if (data.second > score)
			{
				score = data.second;
				best = j;
			}
		}
	}
	else
	{
		m_best = std::nullopt;
	}

	// if nothing found
	if (best == -1)
	{
		std::cout << "no best move found?" << std::endl;
		return;
	}

	m_best = std::optional<checkers::move>(moves[best]);
	
	// debug
	std::cout << "Explored " << extra.exploration << std::endl;
}

void explorer::optimizer::update_board(checkers::board newboard)
{
	m_board = newboard;
}

float explorer::optimizer::get_score() const
{
	return m_score;
}

const std::vector<checkers::move> &explorer::optimizer::get_lines() const
{
	return m_lines;
}

const std::optional<checkers::move> &explorer::optimizer::get_move() const
{
	return m_best;
}
