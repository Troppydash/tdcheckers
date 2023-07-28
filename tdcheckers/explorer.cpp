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
		0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f,
		7.0f, 6.0f, 6.0f, 6.0f, 6.0f, 6.0f, 6.0f, 7.0f,
		7.0f, 5.0f, 5.0f, 5.0f, 5.0f, 5.0f, 5.0f, 7.0f,
		7.0f, 4.0f, 4.0f, 4.0f, 4.0f, 4.0f, 4.0f, 7.0f,
		7.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 7.0f,
		2.0f, 2.0f, 2.0f, 2.0f, 2.0f, 2.0f, 2.0f, 2.0f,
		2.0f, 2.0f, 2.0f, 2.0f, 2.0f, 2.0f, 2.0f, 2.0f,
		2.0f, 2.0f, 2.0f, 2.0f, 2.0f, 2.0f, 2.0f, 2.0f,
	};

	checkers::state other = checkers::state_flip(player);

	uint64_t playerbitboard = board.get_player(player);
	uint64_t otherbitboard = board.get_player(other);
	uint64_t bitboard = playerbitboard | otherbitboard;
	uint64_t kingbitboard = board.get_kings(player) | board.get_kings(other);

	float selfscore = 0.0f;
	float otherscore = 0.0f;

	
	float menweight = 0.4f;
	float menvalueweight = 0.6f;
	float kingweight = 2.0f;

	int pieces = countbits(bitboard);
	if (pieces < 14)
	{
		menweight = 1.4f;
		kingweight = 2.0f;
		menvalueweight = 0.0f;
	}

	for (int i = 0; i < G_CHECKERS_SIZE; ++i)
	{
		// reward:
		//	   men in general    30%
		//     men positioning   10%
		//     king in general   60%

		uint64_t mask = 1ull << i;

		if ((mask & bitboard) == 0)
		{
			continue;
		}

		bool isplayer = (mask & playerbitboard) != 0;
		bool isking = (mask & kingbitboard) != 0;

		float mengenvalue = 0.0f;
		float menvalue = 0.0f;
		float kingvalue = 0.0f;
		if (!isking)
		{
			mengenvalue = 1.0f;
			if ((player == checkers::state::RED) && isplayer || (player == checkers::state::BLACK && !isplayer))
			{
				menvalue = weights[i] / 9.0f;
			}
			else
			{
				menvalue = weights[64 - i - 1] / 9.0f;
			}

		}
		else
		{
			kingvalue = 1.0f;
		}

		if (isplayer)
		{
			selfscore += menvalueweight * menvalue + kingweight * kingvalue + menweight * mengenvalue;
		}
		else
		{
			otherscore += menvalueweight * menvalue + kingweight * kingvalue + menweight * mengenvalue;
		}
	}

	float netmoves = (float)moves.size() - (float)board.compute_moves(checkers::state_flip(turn)).size();

	return selfscore - otherscore + 0.1f * netmoves;
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
			score += 5;
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
			extra.transposition[hash] = { 1000, -1e6 };
			return -1e6;
		}
		else
		{
			extra.transposition[hash] = { 1000, 1e6 };
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
	std::vector<int> indices(moves.size());
	std::iota(indices.begin(), indices.end(), 0);
	std::sort(indices.begin(), indices.end(),
		[&](int a, int b) -> bool {
		return weights[a] > weights[b];
	});

	float value;
	if (maxing)
	{
		value = alpha;

		for (auto i : indices)
		{
			auto &move = moves[i];

			float newvalue = evaluate(
				board.perform_move(move, turn),
				nextturn,
				player,
				depth_remaining - 1,
				alpha,
				beta,
				false,
				extra,
				std::move(cache[i])
			);

			value = std::max(value, newvalue);
		}
	}
	else
	{
		value = beta;

		for (auto i : indices)
		{
			auto &move = moves[i];

			float newvalue = evaluate(
				board.perform_move(move, turn),
				nextturn,
				player,
				depth_remaining - 1,
				alpha,
				beta,
				true,
				extra,
				std::move(cache[i])
			);

			value = std::min(value, newvalue);
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
	int startdepth = 0;
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
			true,
			extra,
			m_board.compute_moves(turn)
		);

		// debug
		std::cout << "At depth " << depth << ", score = " << m_score << std::endl;
		std::cout << "  " << extra.exploration << std::endl;


		/*for (int j = 0; j < moves.size(); ++j)
		{
			uint64_t hash = hashing(m_board.perform_move(moves[j], turn)) ^ std::hash<bool>()(false);
			if (extra.transposition.count(hash) == 0)
				continue;

			auto &data = extra.transposition[hash];
			std::cout << moves[j].str() << " is " << data.second << std::endl;
		}*/

		// print wtf it is thinking
		checkers::board b = m_board;
		checkers::state t = turn;

		bool maxing = true;
		for (int i = 0; i < depth - 1; ++i)
		{

			auto moves = b.compute_moves(t);
			if (maxing)
			{
				int best = -1;
				int score = -1e9;
				for (int j = 0; j < moves.size(); ++j)
				{
					uint64_t hash = hashing(b.perform_move(moves[j], t)) ^ std::hash<bool>()(t != m_player);
					if (extra.transposition.count(hash) == 0)
						continue;

					auto &data = extra.transposition[hash];
					if (data.second > score)
					{
						best = j;
						score = data.second;
					}
				}

				std::cout << moves[best].str() << " ";

				b = b.perform_move(moves[best], t);
				t = checkers::state_flip(t);
				maxing = !maxing;
			}
			else
			{
				int best = -1;
				int score = 1e9;
				for (int j = 0; j < moves.size(); ++j)
				{
					uint64_t hash = hashing(b.perform_move(moves[j], t)) ^ std::hash<bool>()(t != m_player);
					if (extra.transposition.count(hash) == 0)
						continue;

					auto &data = extra.transposition[hash];
					if (data.second < score)
					{
						best = j;
						score = data.second;
					}
				}

				std::cout << moves[best].str() << " ";
				b = b.perform_move(moves[best], t);
				t = checkers::state_flip(t);
				maxing = !maxing;
			}
		
		}
		std::cout << std::endl;

		// exit when the score is sure
		if (abs(m_score) > 50.0f || extra.exploration > 3000000)
			break;
	}


	// save table
	m_transposition = extra.transposition;

	// compute best move
	std::cout << "Moves: " << std::endl;

	int best = -1;
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

	// if nothing found
	if (best == -1)
	{
		std::cout << "no best move found?" << std::endl;
		return;
	}

	// print line


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
