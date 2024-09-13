#include "game.h"

template<typename B, typename P, typename M>
game::game<B, P, M>::game(
	std::unique_ptr<game::analyzer> p1, 
	std::unique_ptr<game::analyzer> p2,
	const B &board, const P &player
)
	: m_a1(p1), m_a2(p2), m_board(board), m_player(player)
{
}


