#pragma once

#include <iostream>
#include "analyzer.h"

namespace game
{

template <typename B, typename P, typename M>
class game
{
public:
	using analyzer = analyzer::analyzer<B, P, M>;

	game(
		std::unique_ptr<analyzer> p1,
		std::unique_ptr<analyzer> p2,
		const B &board,
		const P &player
	);

	// TODO: Implement

protected:
	P m_player;
	B m_board;
	std::unique_ptr<analyzer> m_a1;
	std::unique_ptr<analyzer> m_a2;
};

}
