#pragma once

#include "checkers.h"


namespace testing
{

	// continuously explores the number of moves from the position,
	// follows a BFS approach
	void explore_moves(checkers::board position, checkers::state turn);

	// Randomly play between two sides
	void random_play();

	// Perform a series of moves
	void perform_moves(checkers::board position, checkers::state turn, const std::vector<checkers::move> &moves);

	// Set the ai to play against a player
	void play_against(checkers::board position, checkers::state player, checkers::state turn);

	// Set the ais to play against itself
	void play_itself(checkers::board position, checkers::state turn);

	// Analyze the board position given the turn
	void analyze(checkers::board position, checkers::state turn);
};