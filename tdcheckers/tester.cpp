#include "tester.h"

#include <vector>
#include <unordered_set>

void testing::explore_moves(checkers::board position, checkers::state turn)
{
	std::unordered_set<checkers::board, checkers::board::hash_function> positions{position};

	int depth = 0;
	while (true)
	{
		size_t actions = 0;
		std::unordered_set<checkers::board, checkers::board::hash_function> nextpositions;
		for (auto &board : positions)
		{
			std::vector<checkers::move> moves = position.compute_moves(turn);
			actions += moves.size();

			for (auto &m : moves)
				nextpositions.emplace(board.perform_move(m, turn));
		}

		std::cout << "Depth " << depth << ": " << actions << std::endl;

		// update boards
		positions = nextpositions;

		// update turn
		if (turn == checkers::state::RED)
			turn = checkers::state::BLACK;
		else
			turn = checkers::state::RED;

		depth += 1;
	}




}
