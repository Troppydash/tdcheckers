#include "explorer.h"

#include <numeric>
#include <algorithm>
#include <random>

#include "global.h"

explorer::optimizer::optimizer(checkers::board board, checkers::state turn)
	: m_board(board), m_player(turn), m_score(0), m_best(), m_lines()
{
}

// ds to store some evaluation globals
struct evaluate_extra
{
	std::unordered_map<uint64_t, std::pair<int, float>> transposition;
	size_t exploration;
};


// helper function
template <typename T> int sgn(T val) {
	return (T(0) < val) - (val < T(0));
}

// returns the number of bits in the bitboard
int countbits(uint64_t bitboard)
{
	constexpr auto masks = global::boardmask();
	int count = 0;
	for (int i = 0; i < masks.size; ++i)
	{
		uint64_t mask = masks.masks[i];
		if ((mask & bitboard) != 0)
			count += 1;
	}
	return count;
}


static float heuristic(
	checkers::board board,
	checkers::state turn,
	checkers::state player,
	const std::vector<checkers::move> &moves
)
{

	static float weights[] = {
		7.0f, 8.0f, 9.0f, 9.0f, 9.0f, 9.0f, 8.0f, 7.0f,
		6.0f, 6.0f, 3.0f, 4.0f, 4.0f, 3.0f, 6.0f, 6.0f,
		5.0f, 3.0f, 2.0f, 2.0f, 2.0f, 2.0f, 3.0f, 5.0f,
		4.0f, 5.0f, 1.0f, 1.0f, 1.0f, 1.0f, 5.0f, 4.0f,
		4.0f, 7.0f, 3.0f, 3.0f, 3.0f, 3.0f, 7.0f, 4.0f,
		2.0f, 2.0f, 2.0f, 2.0f, 2.0f, 2.0f, 2.0f, 2.0f,
		2.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 2.0f,
		2.0f, 2.0f, 2.0f, 2.0f, 2.0f, 2.0f, 2.0f, 2.0f,
	};

	checkers::state other = checkers::state_flip(player);

	uint64_t playerbitboard = board.get_player(player);
	uint64_t otherbitboard = board.get_player(other);
	uint64_t bitboard = playerbitboard | otherbitboard;
	uint64_t kingbitboard = board.get_kings(player) | board.get_kings(other);

	float selfscore = 0.0f;
	float otherscore = 0.0f;

	for (int i = 0; i < G_CHECKERS_SIZE; ++i)
	{
		// reward:
		//     men positioning   20%
		//     king in general   30%
		//     available captures  50%

		uint64_t mask = 1ull << i;

		if ((mask & bitboard) == 0)
		{
			continue;
		}

		bool isplayer = (mask & playerbitboard) != 0;
		bool isking = (mask & kingbitboard) != 0;

		float menvalue = 0.0f;
		float kingvalue = 0.0f;
		if (!isking)
		{
			if (player == checkers::state::RED)
			{
				menvalue = weights[i] / 9.0f;
			}
			else
			{
				menvalue = weights[G_BOARDMASKS_SIZE - i - 1] / 9.0f;
			}

		}
		else
		{
			kingvalue = 1.0f;
		}

		if (isplayer)
		{
			selfscore += 0.2 * menvalue + 0.3 * kingvalue;
		}
		else
		{
			otherscore += 0.2 * menvalue + 0.3 * kingvalue;
		}
	}

	/*
	float captures = 0;
	for (auto &move : moves)
	{
		float value = 0.0f;
		for (auto spot : move.captures)
		{
			if ((spot & kingbitboard) != 0)
				value += 1.f;
			else
				value += 0.5f;
		}
		captures += 0.5 * value;
	}


	float othercaptures = 0;
	for (auto &move : board.compute_moves(checkers::state_flip(turn)))
	{
		float value = 0.0f;
		for (auto spot : move.captures)
		{
			if ((spot & kingbitboard) != 0)
				value += 1.0f;
			else
				value += 0.5f;
		}
		othercaptures += 0.5 * value;
	}

	captures = 0.0f;
	othercaptures = 0.0f;
	*/

	return selfscore - (otherscore);

	/*if (turn == player)
	{
		return selfscore - (otherscore);
	}
	else
	{
		return selfscore - (otherscore);

	}*/


	// count piece closeness to end
	/*

	int pieces = countbits(playerbitboard) + countbits(otherbitboard);

	float playervalue = 0.0f;
	float othervalue = 0.0f;

	for (int i = 0; i < G_CHECKERS_SIZE; ++i)
	{
		uint64_t mask = (1ull << i);

		if (mask & kingbitboard)
			continue;

		// add the player's weight
		if (mask & playerbitboard)
		{
			playervalue += weights[i];
		}
		
		// add the other's weight
		if (mask & otherbitboard)
		{
			int flipped_i = G_CHECKERS_SIZE - i - 1;
			othervalue += weights[flipped_i];
		}
	}


	float piecevalue = playervalue - othervalue;
	

	//// the more pieces, the better
	int diff = countbits(board.get_player(player)) - countbits(board.get_player(other));

	// the more kings, the better
	int kingdiff = countbits(board.get_kings(player)) - countbits(board.get_kings(other));

	// the better captures
	uint64_t kings = board.get_kings(other) | board.get_kings(player);
	float captures = 0;
	for (auto &move : moves)
	{
		for (auto spot : move.captures)
		{
			if ((spot & kings) != 0)
				captures += 2.1f;
			else
				captures += 0.8f;
		}
	}

	float othercaptures = 0;
	for (auto &move : board.compute_moves(checkers::state_flip(turn)))
	{
		for (auto spot : move.captures)
		{
			if ((spot & kings) != 0)
				othercaptures += 2.1f;
			else
				othercaptures += 0.8f;
		}
	}


	float scale = (turn == player) ? 1.0f : -1.0f;

	if (pieces < 14)
	{
		return 2.6f * diff + + 8.0f * kingdiff + 1.8f * scale * (captures - othercaptures);
	}

	return 0.6f * diff + 0.1f * piecevalue + 5.0f * kingdiff + 1.8f * scale * (captures - othercaptures);
	*/
}

static std::vector<float> weight_moves(
	checkers::board board, 
	const std::vector<checkers::move> &moves, 
	checkers::state turn,
	checkers::state player,
	std::vector<std::vector<checkers::move>> &cache,
	evaluate_extra &extra
)
{
	std::vector<float> out;
	out.reserve(moves.size());

	checkers::state other = checkers::state_flip(turn);

	uint64_t kings = board.get_kings(turn);

	// top hash move
	const checkers::move *topmove = nullptr;
	float score = -1e9;
	for (auto &move : moves)
	{
		uint64_t hashed = checkers::board::hash_function()(board.perform_move(move, turn)) ^ std::hash<bool>()(turn != player);
		if (extra.transposition.count(hashed) != 0)
		{
			float value = extra.transposition[hashed].second;
			if (value > score)
			{
				score = value;
				topmove = &move;
			}
		}
	}


	for (auto &move : moves)
	{
		float score = 0.0f;

		// king promotions are good
		if (move.king && (kings & move.from) == 0)
			score += 0.5f;

		// king moves are good
		if (move.king)
			score += 0.02f;

		// captures are good
		if (move.captures.size() == 1)
			score += 0.2f;
		else
			score += 0.3f * move.captures.size();

		// encourage jumping into others
		checkers::board after = board.perform_move(move, turn);
		auto aftermoves = after.compute_moves(other);
		size_t aftercaptures = 0;
		for (auto &m : aftermoves)
		{
			aftercaptures += m.captures.size();
		}
		score -= 0.2f * aftercaptures;

		cache.push_back(std::move(aftermoves));

		if (topmove != nullptr && move == *topmove)
		{
			score += 10;
		}
		out.push_back(score);
	}

	return out;
}

// alpha-beta evaluation function
static float evaluate(
	checkers::board board,
	checkers::state turn,
	checkers::state player,
	int depth_remaining,
	float alpha, float beta,
	bool maxing,
	// extra
	evaluate_extra &extra,
	std::vector<checkers::move> &&precalc
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

	std::vector<checkers::move> moves = std::move(precalc);
	if (moves.size() == 0)
	{
		if (turn == player)
		{
			extra.transposition[hash] = { 100, -1e6 };
			return -1e6;
		}
		else
		{
			extra.transposition[hash] = { 100, 1e6 };
			return 1e6;
		}
	}

	if (depth_remaining == 0)
	{
		float score = heuristic(board, turn, player, moves);
		extra.transposition[hash] = { 0, score };
		return score;
	}


	checkers::state nextturn = checkers::state_flip(turn);

	// move ordering
	std::vector<std::vector<checkers::move>> cache;
	auto weights = weight_moves(board, moves, turn, player, cache, extra);

	// sort moves based on weights, desc
	std::vector<int> indices(weights.size());
	std::iota(indices.begin(), indices.end(), 0);
	std::sort(indices.begin(), indices.end(),
		[&](int a, int b) -> bool {
		return weights[a] > weights[b];
	});

	float value;
	if (maxing)
	{
		value = -1e9;

		for (auto &i : indices)
		{
			auto &move = moves[i];
			int remaining = depth_remaining - 1;
			if (move.captures.size() >= 1 && remaining <= move.captures.size())
			{
				extra.exploration -= 1;
				remaining += 1;
			}
			

			float newvalue = evaluate(
				board.perform_move(move, turn),
				nextturn,
				player,
				remaining,
				alpha,
				beta,
				false,
				extra,
				std::move(cache[i])
			);

			if (newvalue > value)
				value = newvalue;

			if (value > alpha)
				alpha = value;


			if (value > beta)
				break;

		}
	}
	else
	{
		value = 1e9;

		for (auto &i : indices)
		{
			auto &move = moves[i];
			int remaining = depth_remaining - 1;
			if (move.captures.size() >= 1 && remaining <= move.captures.size())
			{
				extra.exploration -= 1;
				remaining += 1;
			}

			float newvalue = evaluate(
				board.perform_move(move, turn),
				nextturn,
				player,
				remaining,
				alpha,
				beta,
				true,
				extra,
				std::move(cache[i])
			);

			if (newvalue < value)
				value = newvalue;

			if (value < beta)
				beta = value;

			if (value < alpha)
				break;
		}
	}

	if (value != 0)
		value -= 0.01f * sgn(value);

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

	std::random_device dev;
	std::mt19937 rng(dev());
	std::uniform_real_distribution<float> dist(0.0, 1.0);


	evaluate_extra extra{
		m_transposition,
		0
	};

	m_score = 0;

	auto moves = m_board.compute_moves(turn);
	checkers::state nextturn = checkers::state_flip(turn);
	auto hashing = checkers::board::hash_function();

	// iterative deepining
	int startdepth = 8;
	int enddepth = 16;
	for (int depth = startdepth; depth < enddepth; ++depth)
	{
		extra.exploration = 0;

		m_score = evaluate(
			m_board,
			turn,
			m_player,
			depth,
			-1e9,
			1e9,
			m_player == turn,
			extra,
			m_board.compute_moves(turn)
		);

		// debug
		std::cout << "At depth " << depth << ", score = " << m_score << std::endl;
		std::cout << "  " << extra.exploration << std::endl;

		// exit when the score is sure
		if (abs(m_score) > 50.0f || extra.exploration > 3000000)
			break;
	}


	// save table
	m_transposition = extra.transposition;

	// compute best move
	std::cout << "Moves: " << std::endl;

	bool maxing = turn == m_player;
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
			std::cout << moves[j].str() << " is " << data.second << std::endl;
			if (data.second > score || (dist(rng) < 0.3f && data.second == score))
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
