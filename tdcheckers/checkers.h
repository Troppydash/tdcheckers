#pragma once



#include <string>
#include <vector>
#include <iostream>


/*
	Header file implementing the checkers game


	House checkers rules:
	Board
		Bottom left is occupied, denoted as black tile
		12 pieces on each side in a diagonal, crisscross pattern
		8x8 board

	Gameplay
		Red, bottom player starts
		Movement:
			For regular pieces (pieces) forward diagonal movement only
			For kings, any direction single diagonal movement only

		Captures:
			For pieces, jumping across an opponent piece, can be chained in the forward direction only
			For kings, jumping across an opponent piece, can be chained in any angle

	Termination
		On their move,
			The player with no legal move loses
			The player with no piece loses
		Drawing is occured with 3 fold repetition and 40 moves with no captures





*/

#define G_CHECKERS_WIDTH (8)
#define G_CHECKERS_SIZE (8*8)
#define G_REDPIECE ('o')
#define G_BLACKPIECE ('x')
#define G_BLANKPIECE ('.')

namespace checkers
{
	
	enum class state
	{
		RED = 0,
		BLACK,
		DRAW,
		NONE,
	};

	// return the string representation of the state
	std::string state_repr(state s);
	state state_flip(state s);


	struct move
	{
		// the start of the move
		uint64_t from;

		// the end of the move
		uint64_t to;

		// the captures made by the piece
		std::vector<uint64_t> captures;

		// whether the piece became a king (not if it was a king)
		bool king;

		move()
			: from(0ull), to(0ull), captures({}), king(false)
		{}


		// default constructor
		move(uint64_t from, uint64_t to, std::vector<uint64_t> captures, bool king);

		// shorthand string constructor
		move(std::string text);

		// equality overload
		bool operator==(const move &other) const
		{
			if (from != other.from)
				return false;

			if (to != other.to)
				return false;

			if (king != other.king)
				return false;

			if (captures.size() != other.captures.size())
				return false;

			for (int i = 0; i < captures.size(); ++i)
			{
				if (captures[i] != other.captures[i])
					return false;
			}

			return true;
		}

		// returns the string representation of the move
		std::string repr() const;

		// returns the shorthand string representation of the move
		std::string str() const;
	};





	class board
	{
	public:
		// default constructor
		board();

		// copy constructor
		board(uint64_t red, uint64_t black, uint64_t kings);

		// string constructor
		board(std::string text);

		// moves constructor from initial positions
		board(std::vector<move> &moves);

		// copy the state of another board to self
		void copy(const board &other);

		// returns the string stylized representation of the board and its states
		std::string repr() const;

		// returns a list of available moves given the current player
		std::vector<move> compute_moves(state turn) const;

		// performs the given move based on the current player, returns a new board where the move is performed
		board perform_move(const checkers::move &move, state turn) const;

		// returns the board state given the current player turn
		// note: this does not handle drawing yet
		state get_state(state turn) const;

		bool operator==(const board &other) const
		{
			return m_red == other.m_red && m_black == other.m_black && m_kings == other.m_kings;
		}

		// implementing hashing for unordered_set
		struct hash_function
		{
			// a functor representing the hash function of our board
			size_t operator()(const board &board) const
			{
				size_t h1 = std::hash<uint64_t>()(board.m_red | (board.m_black >> 1));
				size_t h3 = std::hash<uint64_t>()(board.m_kings);

				return h1 ^ (h3 << 1);
			}
		};

		uint64_t get_player(state player) const;
		uint64_t get_kings(state player) const;

	private:
		// the red player's bit mask
		uint64_t m_red;

		// the black player's bit mask
		uint64_t m_black;

		// the king's (of both player) bit mask
		uint64_t m_kings;
	};
}

