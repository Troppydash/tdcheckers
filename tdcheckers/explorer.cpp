#include "explorer.h"

#include <numeric>
#include <algorithm>
#include <random>
#include <thread>
#include <mutex>

#include "global.h"

explorer::optimizer::optimizer(checkers::board board, checkers::state turn)
	: m_board(board), m_player(turn), m_score(0), m_best(), m_lines()
{
}

// ds to store some evaluation globals
struct evaluate_extra
{
	// lock for the transposition table
	std::mutex lock;

	// the transposition table
	explorer::transpositiontable transposition;

	// number of board positions explored
	size_t exploration;

	std::optional<checkers::move> best;
};


// helper function to get the sign of the function
template <typename T>
int sgn(T val) {
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


// Returns the hueristic evaluation of the board
static float heuristic(
	checkers::board board,
	checkers::state turn,
	checkers::state player
)
{
	static float weights[] = {
		0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f,
		7.0f, 6.0f, 6.0f, 6.0f, 6.0f, 6.0f, 6.0f, 7.0f,
		7.0f, 5.0f, 5.0f, 5.0f, 5.0f, 5.0f, 5.0f, 7.0f,
		7.0f, 4.0f, 4.0f, 4.0f, 4.0f, 4.0f, 4.0f, 7.0f,
		9.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 9.0f,
		7.0f, 2.0f, 2.0f, 2.0f, 2.0f, 2.0f, 2.0f, 7.0f,
		7.0f, 2.0f, 2.0f, 2.0f, 2.0f, 2.0f, 2.0f, 7.0f,
		5.0f, 5.0f, 5.0f, 5.0f, 5.0f, 5.0f, 5.0f, 5.0f,
	};

	static float endweights[] = {
		0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f,
		0.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 0.0f,
		0.0f, 1.0f, 2.0f, 2.0f, 2.0f, 2.0f, 1.0f, 1.0f,
		0.0f, 1.0f, 2.0f, 3.0f, 3.0f, 2.0f, 1.0f, 1.0f,
		0.0f, 1.0f, 2.0f, 3.0f, 3.0f, 2.0f, 1.0f, 1.0f,
		0.0f, 1.0f, 2.0f, 2.0f, 2.0f, 2.0f, 1.0f, 1.0f,
		0.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 0.0f,
		0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f,
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
		menweight = 0.4f;
		kingweight = 2.0f;
		menvalueweight = 0.6f;
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
			/*kingvalue = 1.0f;
			if (pieces < 14)*/
			{
				if ((player == checkers::state::RED) && isplayer || (player == checkers::state::BLACK && !isplayer))
				{
					kingvalue = (endweights[i] + 1.0f) / 4.0f;
				}
				else
				{
					kingvalue = (endweights[64 - i - 1] + 1.0f) / 4.0f;
				}
			}

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

	//float netmoves = (float)moves.size() - (float)board.compute_moves(checkers::state_flip(turn)).size();

	return selfscore - otherscore;
}

// Returns the weighting of the moves
static std::vector<float> weight_moves(
	checkers::board board,
	const std::vector<checkers::move> &moves,
	checkers::state turn,
	checkers::state player,
	evaluate_extra &extra,
	bool maxing
)
{
	std::vector<float> out;
	out.reserve(moves.size());

	checkers::state other = checkers::state_flip(turn);

	extra.lock.lock();
	for (auto &move : moves)
	{
		checkers::board newboard = board.perform_move(move, turn);

		float caps = move.captures.size();
		if (!maxing)
			caps *= -1.0f;

		// top move bonus
		uint64_t hash = checkers::board::hash_function()(newboard) ^ std::hash<bool>()(other == player);
		if (extra.transposition.count(hash) == 0)
		{
			float heur = heuristic(newboard, other, player);
			out.push_back(0.8f * heur + caps);
		}
		else
		{
			out.push_back(extra.transposition[hash].value + caps);
		}
	}
	extra.lock.unlock();

	return out;
}

// alpha-beta evaluation function (or at least, it should be)
// whether this is the top level call
template <bool TOP>
static float evaluate(
	// the current board state
	checkers::board board,
	// the turn
	checkers::state turn,
	// the player to compute score against
	checkers::state player,
	// depth search remaining
	int depth_remaining,
	// alpha beta values
	float alpha, float beta,
	// whether to min or max
	bool maxing,
	// extra info
	evaluate_extra &extra,
	// precalculated moves
	std::vector<checkers::move> &&precalc
)
{
	extra.exploration += 1;

	// use transposition
	uint64_t hash = checkers::board::hash_function()(board) ^ std::hash<bool>()(turn == player);

	// transposition lookup
	if (!TOP)
	{
		extra.lock.lock();
		if (extra.transposition.count(hash) != 0)
		{
			auto &data = extra.transposition[hash];

			// only return the transposition if the stored depth is higher than remaining
			if (data.depth >= depth_remaining)
			{
				if (data.alpha >= beta)
				{
					extra.lock.unlock();
					return data.alpha;
				}

				if (data.beta <= alpha)
				{
					extra.lock.unlock();
					return data.beta;
				}

				alpha = std::max(alpha, data.alpha);
				beta = std::min(beta, data.beta);
			}
		}
		extra.lock.unlock();
	}



	std::vector<checkers::move> moves = std::move(precalc);
	if (moves.size() == 0)
	{
		if (turn == player)
		{
			/*extra.lock.lock();
			extra.transposition[hash] = { 1000, -1e6, 100 };
			extra.lock.unlock();*/
			return -1e6;
		}
		else
		{
			/*extra.lock.lock();
			extra.transposition[hash] = { 1000, 1e6, 100 };
			extra.lock.unlock();*/
			return 1e6;
		}
	}


	if (depth_remaining == 0)
	{
		float score = heuristic(board, turn, player);
		/*extra.lock.lock();
		extra.transposition[hash] = { 0, score, 2 };
		extra.lock.unlock();*/

		return score;
	}


	checkers::state nextturn = checkers::state_flip(turn);

	// move ordering
	std::vector<int> indices(moves.size());
	std::iota(indices.begin(), indices.end(), 0);
	if (!TOP)
	{
		// sort moves based on weights, desc
		auto weights = weight_moves(board, moves, turn, player, extra, maxing);
		std::sort(indices.begin(), indices.end(), [&](int a, int b)
		{
			if (maxing)
			{
				return weights[a] > weights[b];
			}
			return weights[a] < weights[b];
		});
	}

	float value;
	if (maxing)
	{
		value = alpha;
		float a = alpha;

		if (TOP)
		{
			std::vector<float> evals;
			auto eval = [&](int i)
			{
				auto &move = moves[i];

				float newvalue = evaluate<false>(
					board.perform_move(move, turn),
					nextturn,
					player,
					depth_remaining - 1,
					alpha,
					beta,
					false,
					extra,
					board.perform_move(move, turn).compute_moves(nextturn)
				);

				evals[i] = newvalue;
			};

			std::vector<std::thread> threads;
			for (int i = 0; i < moves.size(); ++i)
			{
				evals.push_back(0.0f);
				threads.push_back(std::thread{ eval, i });
			}


			for (int i = 0; i < moves.size(); ++i)
			{
				threads[i].join();
			}

			for (int i = 0; i < moves.size(); ++i)
			{
				if (evals[i] > value)
				{
					value = evals[i];
					extra.best = moves[i];
				}
			}
		}
		else
		{
			for (auto i : indices)
			{
				auto &move = moves[i];

				float newvalue = evaluate<false>(
					board.perform_move(move, turn),
					nextturn,
					player,
					depth_remaining - 1,
					a,
					beta,
					false,
					extra,
					board.perform_move(move, turn).compute_moves(nextturn)
				);

				value = std::max(value, newvalue);
				a = std::max(a, newvalue);
				if (beta <= a)
					break;
			}
		}
	}
	else
	{
		value = beta;
		float b = beta;

		for (auto i : indices)
		{
			auto &move = moves[i];
			/*int newdepth = depth_remaining - 1;
			if (move.captures.size() > 0 && newdepth == 0)
			{
				newdepth = 1;
			}*/

			float newvalue = evaluate<false>(
				board.perform_move(move, turn),
				nextturn,
				player,
				depth_remaining - 1,
				alpha,
				b,
				true,
				extra,
				board.perform_move(move, turn).compute_moves(nextturn)
			);

			value = std::min(value, newvalue);
			b = std::min(b, newvalue);
			if (b <= alpha)
				break;
		}
	}

	// update transposition
	extra.lock.lock();

	// add the entry if it exists
	if (extra.transposition.count(hash) == 0)
	{
		extra.transposition[hash] = { depth_remaining, -1e9, 1e9, value, 4 };
	}
	else
	{
		// ignore the saving if depth is too low?
		if (extra.transposition[hash].depth > depth_remaining)
		{
			extra.lock.unlock();
			return value;
		}

		extra.transposition[hash].value = value;
		extra.transposition[hash].depth = depth_remaining;
	}

	auto &data = extra.transposition[hash];
	// fail low
	if (value <= alpha)
	{
		data.beta = value;
	}

	// fail within range
	if (value > alpha && value < beta)
	{
		data.alpha = value;
		data.beta = value;
	}

	// fail high
	if (value >= beta)
	{
		data.alpha = value;
	}

	extra.lock.unlock();

	return value;
}

// TODO! Fix this shit
static float MTDF(
	// the current board state
	checkers::board board,
	// the turn
	checkers::state turn,
	// the player to compute score against
	checkers::state player,
	// depth search remaining
	int depth_remaining,
	// extra info
	evaluate_extra &extra,
	float best
)
{
	float g = best;
	float upperbound = 1e9;
	float lowerbound = -1e9;
	while (lowerbound + 0.00001f < upperbound)
	{
		float beta = std::max(g, lowerbound + 0.1f);
		//std::cout << g << "," << lowerbound << "," << upperbound << std::endl;
		g = evaluate<true>(
			board,
			turn,
			player,
			depth_remaining,
			beta - 1.0f,
			beta,
			true,
			extra,
			board.compute_moves(turn)
		);

		if (g < beta)
			upperbound = g;
		else
			lowerbound = g;
	}

	return g;
}


void explorer::optimizer::compute_score(checkers::state turn, bool verbose)
{
	// purge transposition table entries that ran out of age
	auto it = m_transposition.begin();
	while (it != m_transposition.end())
	{
		// decrement age
		it->second.age -= 1;

		// remove the element if it ran out of age
		if (it->second.age <= 0)
		{
			m_transposition.erase(it++);
		}
		else
		{
			++it;
		}
	}

	// scores follow the definition
	// + for red winning
	// - for black winning
	// the higher the |score|, the larger the advantage
	// 0 is even

	/*std::random_device dev;
	std::mt19937 rng(dev());
	std::uniform_real_distribution<float> dist(0.0, 1.0);*/


	// set extra data
	evaluate_extra extra{
		{},
		m_transposition,
		0,
		std::nullopt
	};

	// compute moves and other temporary constants
	auto moves = m_board.compute_moves(turn);
	checkers::state nextturn = checkers::state_flip(turn);
	auto hashing = checkers::board::hash_function();

	if (verbose)
		std::cout << "---- Depths ----" << std::endl;

	// reset score (do not use the last iter's it will break)
	m_score = 0;
	// iterative deepining
	int startdepth = 1;
	int enddepth = 16;
	for (int depth = startdepth; depth < enddepth; ++depth)
	{
		extra.exploration = 0;
		//if (verbose)
		//{
		/*	m_score = MTDF(
				m_board,
				turn,
				m_player,
				depth,
				extra,
				m_score
			);*/
		//}
		//else
		{
			m_score = evaluate<true>(
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
		}


		if (verbose && depth >= 8)
		{
			std::cout << "At " << depth << ", score = " << m_score << std::endl;
			std::cout << "  " << extra.exploration << std::endl;
			//std::cout << "-- best line --" << std::endl;
		}

		// print wtf it is thinking, print the best line
		checkers::board b = m_board;
		checkers::state t = turn;

		bool maxing = true;
		for (int i = 0; i < depth - 1; ++i)
		{
			auto moves = b.compute_moves(t);
			if (maxing)
			{
				int best = -1;
				float score = -1e9;
				for (int j = 0; j < moves.size(); ++j)
				{
					uint64_t hash = hashing(b.perform_move(moves[j], t)) ^ std::hash<bool>()(t != m_player);
					if (extra.transposition.count(hash) == 0)
						continue;

					auto &data = extra.transposition[hash];
					if (data.value > score)
					{
						best = j;
						score = data.value;
					}
				}

				if (best == -1)
					break;

				if (verbose && depth >= 8)
					std::cout << moves[best].str() << " ";

				b = b.perform_move(moves[best], t);
				t = checkers::state_flip(t);
				maxing = !maxing;
			}
			else
			{
				int best = -1;
				float score = 1e9;
				for (int j = 0; j < moves.size(); ++j)
				{
					uint64_t hash = hashing(b.perform_move(moves[j], t)) ^ std::hash<bool>()(t != m_player);
					if (extra.transposition.count(hash) == 0)
						continue;

					auto &data = extra.transposition[hash];
					if (data.value < score)
					{
						best = j;
						score = data.value;
					}
				}

				if (best == -1)
					break;

				if (verbose && depth >= 8)
					std::cout << moves[best].str() << " ";

				b = b.perform_move(moves[best], t);
				t = checkers::state_flip(t);
				maxing = !maxing;
			}
		}

		if (verbose && depth >= 8)
			std::cout << "\n\n";

		// exit when the score is sure
		int limit = 100000;
		if (abs(m_score) > 100.0f || extra.exploration > limit)
		{
			std::cout << "Cutoff depth " << depth << "\n";
			break;
		}
	}


	// save table
	m_transposition = extra.transposition;

	// compute best move
	if (verbose)
		std::cout << "\n----- Evaluations -----" << std::endl;

	m_best = extra.best;

	for (int j = 0; j < moves.size(); ++j)
	{
		uint64_t hash = hashing(m_board.perform_move(moves[j], turn)) ^ std::hash<bool>()(false);
		if (extra.transposition.count(hash) == 0)
			continue;

		auto &data = extra.transposition[hash];
		if (verbose)
			std::cout << moves[j].str() << " is " << data.value << std::endl;
	}

	return;
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
