#pragma once

namespace analyzer
{

/// Generic Analyzer type that is blocking
template <typename B, typename P, typename M>
class analyzer
{
public:
	// returns the score of the board where player is to move next, positive indicates player 1 is winning
	virtual double compute_score(const B &board, const P &player) const;

	// returns the best move of the player
	virtual M best_move(const B &board, const P &player) const;
};

}
