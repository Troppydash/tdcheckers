#pragma once

#include <unordered_map>
#include <optional>
#include "checkers.h"

namespace explorer
{

	struct ttable
	{
		// depth of search done
		int depth;
		// lower bound
		float alpha;
		// upper bound
		float beta;
		// age of this entry (entry deleted when age <= 0)
		float value;
		int age;
	};
	using transpositiontable = std::unordered_map<uint64_t, ttable>;

	// this is an continuous optimizer
	class optimizer
	{
	public:
		// initialization code
		optimizer(checkers::board board, checkers::state player);

		// runs the computation of the position scores
		// implicitly sets the values in the optimizer state
		void compute_score(checkers::state turn, bool verbose = true);

		void update_board(checkers::board newboard);

		float get_score() const;

		// this is currently broken
		const std::vector<checkers::move> &get_lines() const;

		const std::optional<checkers::move> &get_move() const;

	private:
		checkers::board m_board;
		checkers::state m_player;

	private:
		// state variables
		std::optional<checkers::move> m_best;
		float m_score;
		std::vector<checkers::move> m_lines;
		transpositiontable m_transposition;
	};



};