#pragma once

#include "checkers.h"


namespace testing
{

	// continuously explores the number of moves from the position,
	// follows a BFS approach
	void explore_moves(checkers::board position, checkers::state turn);

	void random_play();

	void perform_moves(checkers::board position, checkers::state turn, const std::vector<checkers::move> &moves);

	void play_against(checkers::board position, checkers::state player, checkers::state turn);

	void play_itself(checkers::board position, checkers::state turn);

	void analyze(checkers::board position, checkers::state turn);
};