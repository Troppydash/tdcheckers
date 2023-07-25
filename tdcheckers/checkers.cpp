#include "checkers.h"

#include <bitset>

// helper that returns a checkers bitboard from [rowstart, rowend]
static uint64_t make_checkers_bitboard(int rowstart, int rowend)
{
	uint64_t bitboard = 0ull;

	for (int row = 0; row < G_CHECKERS_WIDTH; ++row)
	{
		// offset such that even rows has 1 offset, and odd rows has 0
		int offset = 1 - (row % 2);

		// filter rows
		if (!(row >= rowstart && row <= rowend))
		{
			continue;
		}

		// append the bits on the row
		for (int i = offset; i < G_CHECKERS_WIDTH; i += 2)
		{
			bitboard |= (1ull << (i + row * G_CHECKERS_WIDTH));
		}
	}

	return bitboard;
}

// returns a string representation of the bitboard
static std::string bitboard_repr(uint64_t bitboard)
{
	std::string out;

	// fill the out buffer
	for (int i = 0; i < G_CHECKERS_SIZE; ++i)
	{
		// decide what the piece is on the mask
		uint64_t mask = 1ull << i;
		char piece;
		if (bitboard & mask)
			piece = G_REDPIECE;
		else
			piece = G_BLANKPIECE;

		// set the output
		out += piece;

		// add newline
		if (i % G_CHECKERS_WIDTH == (G_CHECKERS_WIDTH - 1))
		{
			out += '\n';
		}
	}

	return out;
}

// split the bits in a bitboard into an array of individual bitboards
static std::vector<uint64_t> splitbits(uint64_t board)
{
	std::vector<uint64_t> bits;
	for (int i = 0; i < G_CHECKERS_SIZE; ++i)
	{
		uint64_t mask = 1ull << i;
		if (!(board & mask))
			continue;

		bits.push_back(mask);
	}

	return bits;
}


// converts a coordinate position to a bitboard
static uint64_t coordtoboard(int row, int col)
{
	return 1ull << (row * G_CHECKERS_WIDTH + col);
}

// converts a bitboard position to a pair of coordinates
static std::pair<int, int> boardtocoord(uint64_t board)
{
	for (int i = 0; i < G_CHECKERS_SIZE; ++i)
	{
		if (board & (1ull << i))
		{
			return { i / G_CHECKERS_WIDTH, i % G_CHECKERS_WIDTH };
		}
	}

	// exception
}

// default constructor
checkers::move::move(uint64_t from, uint64_t to, std::vector<uint64_t> captures, bool king)
	: from(from), to(to), captures(captures), king(king)
{}

// shorthand string constructor
checkers::move::move(std::string text)
{
	// has the format
	// o:31,42
	// o:31,x42,53

	// parse initial
	bool isking = text[0] == 'k';

	int row = G_CHECKERS_WIDTH - (text[2] - '0');
	int col = (text[3] - '0') - 1;

	from = coordtoboard(row, col);
	king = isking;
	to = 0ull;  // temp value

	int j = 4;
	while (text[j] == ',')
	{
		int n = text[j + 1];
		if (n != 'x')
		{
			int torow = G_CHECKERS_WIDTH - (text[j + 1] - '0');
			int tocol = (text[j + 2] - '0') - 1;

			to = coordtoboard(torow, tocol);
			break;
		}

		int torow = G_CHECKERS_WIDTH - (text[j + 2] - '0');
		int tocol = (text[j + 3] - '0') - 1;
		captures.push_back(coordtoboard(torow, tocol));
		j += 4;
	}
}

// returns the string representation of the move
std::string checkers::move::repr() const
{
	uint64_t out = from | to;
	std::string text = bitboard_repr(out);

	if (captures.size() > 0)
		text += "x";
	else
		text += "o";

	if (king)
		text += "k";
	else
		text += "o";

	return text;
}

// returns the shorthand string representation of the move
std::string checkers::move::str() const
{
	std::string out;
	if (king)
		out += "k";
	else
		out += "o";

	out += ":";

	std::pair<int, int> f = boardtocoord(from);
	out += (G_CHECKERS_WIDTH - f.first) + '0';
	out += f.second + '1';

	for (auto cap : captures)
	{
		out += ",x";
		std::pair<int, int> f = boardtocoord(cap);
		out += (G_CHECKERS_WIDTH - f.first) + '0';
		out += f.second + '1';
	}

	out += ",";
	std::pair<int, int> t = boardtocoord(to);
	out += (G_CHECKERS_WIDTH - t.first) + '0';
	out += t.second + '1';

	return out;
}



// compute all the jumps available for the given piece, recursively
// for the root call:
//   out is the output moves
//   captures is the stack of captures
//   origin is the root call piece position
//   at is the current, backtracking piece position to calculate future captures from
//   player is the bitmask for the player's pieces
//   other is the backtracking other player's pieces
//   direction is the direction headed by the piece, updated during promotions
static void checkers_compute_jumps(
	std::vector<checkers::move> &out,
	std::vector<uint64_t> captures,
	uint64_t origin,
	uint64_t at,
	uint64_t player, uint64_t other,
	int direction
)
{
	uint64_t boardmask = make_checkers_bitboard(0, G_CHECKERS_WIDTH - 1);

	// check four diagonals
	int width = G_CHECKERS_WIDTH;
	int forward[2] = { (width - 1), (width + 1) };
	int backward[2] = { -(width + 1), -(width - 1) };

	std::vector<int> shifts;
	if (direction == -1)
	{
		shifts = { backward[0], backward[1] };
	}
	else if (direction == 1)
	{
		shifts = { forward[0], forward[1] };
	}
	else
	{
		shifts = { backward[0], backward[1], forward[0], forward[1] };
	}

	// howtf this going around corners
	for (auto shift : shifts)
	{

		// create mask
		uint64_t mask;
		if (shift < 0)
			mask = at >> (-shift);
		else
			mask = at << shift;


		// cannot jump without an enemy piece
		if ((other & mask) == 0)
			continue;


		// check the second layer
		uint64_t secondmask;
		if (shift < 0)
			secondmask = at >> (2 * -shift);
		else
			secondmask = at << (2 * shift);

		// make sure the second move is on the board
		secondmask &= boardmask;

		// if occupied, or off the board,  continue
		if ((player | other) & secondmask || secondmask == 0)
			continue;


		// else consider this jump
		std::vector<uint64_t> newcaptures(captures);
		newcaptures.push_back(mask);

		out.push_back({ origin, secondmask, newcaptures, direction == 0 });

		// change to king if reached the end
		uint64_t toprow = make_checkers_bitboard(width - 1, width - 1);
		uint64_t bottomrow = make_checkers_bitboard(0, 0);
		if (direction == 1 && secondmask & toprow)
			direction = 0;
		else if (direction == -1 && secondmask & bottomrow)
			direction = 0;

		// iterate, delete other piece, shouldn't have to worry about player pieces
		other ^= mask;
		checkers_compute_jumps(out, newcaptures, origin, secondmask, player, other, direction);
	}
}

// default constructor
checkers::board::board()
{
	// red is from row [last-2, last]
	uint64_t red = make_checkers_bitboard(G_CHECKERS_WIDTH - 1 - 2, G_CHECKERS_WIDTH - 1);
	// black is from row [0, 2]
	uint64_t black = make_checkers_bitboard(0, 2);

	m_red = red;
	m_black = black;
	m_kings = 0ull;
}

// copy constructor
checkers::board::board(uint64_t red, uint64_t black, uint64_t kings)
	: m_red(red), m_black(black), m_kings(kings)
{}

// string constructor
checkers::board::board(std::string text)
{
	// assume that text is of format
	// .x.x.x.x
	// x.x.x.x.
	// ...
	//
	// where
	// x = black, o = red
	// cap for kings

	// strip spaces
	std::string stripped;
	for (auto c : text)
	{
		if (!isspace(c))
			stripped += c;
	}


	m_red = 0ull;
	m_black = 0ull;
	m_kings = 0ull;

	// fill the bitboards using the flipped text
	for (int i = 0; i < G_CHECKERS_SIZE; ++i)
	{
		char key = stripped[i];
		uint64_t mask = 1ull << i;

		switch (tolower(key))
		{

		case G_BLANKPIECE:
			break;

		case G_REDPIECE:
		{
			m_red |= mask;
			break;
		}

		case G_BLACKPIECE:
		{
			m_black |= mask;
			break;
		}

		// ignore lmao
		default:
			break;
		}

		if (isupper(key))
		{
			m_kings |= mask;
		}
	}
}

// moves constructor from initial positions
checkers::board::board(std::vector<move> &moves)
{
	state turn = state::RED;

	// play the moves
	for (auto &move : moves)
	{
		board newboard = perform_move(move, turn);
		// copy board
		copy(newboard);
	}
}

// copy the state of another board to self
void checkers::board::copy(const board &other)
{
	m_red = other.m_red;
	m_black = other.m_black;
	m_kings = other.m_kings;
}

// returns the string stylized representation of the board and its states
std::string checkers::board::repr() const
{
	std::string out;


	// fill the out buffer
	for (int i = 0; i < G_CHECKERS_SIZE; ++i)
	{
		if (i % G_CHECKERS_WIDTH == 0)
		{
			out += std::to_string(8 - (i / G_CHECKERS_WIDTH));
			out += " ";
		}

		// decide what the piece is on the mask
		uint64_t mask = 1ull << i;
		char piece;
		if (m_red & mask)
			piece = G_REDPIECE;
		else if (m_black & mask)
			piece = G_BLACKPIECE;
		else
			piece = G_BLANKPIECE;

		if (m_kings & mask)
			piece = toupper(piece);

		// set the output
		out += piece;
		out += ' ';

		// add newline
		if (i % G_CHECKERS_WIDTH == (G_CHECKERS_WIDTH - 1))
		{
			out += '\n';
		}
	}

	out += "  ";
	for (int c = 0; c < G_CHECKERS_WIDTH; ++c)
	{
		out += std::to_string(c + 1);
		out += " ";
	}
	out += "\n";

	return out;
}

// returns a list of available moves given the current player
std::vector<checkers::move> checkers::board::compute_moves(state turn) const
{
	uint64_t toprow = make_checkers_bitboard(G_CHECKERS_WIDTH - 1, G_CHECKERS_WIDTH - 1);
	uint64_t bottomrow = make_checkers_bitboard(0, 0);

	// retrieve the bitboards
	uint64_t player, other;
	uint64_t promotion;
	int direction;

	if (turn == state::RED)
	{
		player = m_red;
		other = m_black;
		direction = -1;
		promotion = bottomrow;
	}
	else
	{
		player = m_black;
		other = m_red;
		direction = 1;
		promotion = toprow;
	}

	std::vector<move> moves;

	// helpers
	uint64_t boardmask = make_checkers_bitboard(0, G_CHECKERS_WIDTH - 1);
	int width = G_CHECKERS_WIDTH;

	// for each player piece
	for (int i = 0; i < G_CHECKERS_SIZE; ++i)
	{
		uint64_t mask = (1ull << i);

		// return if not the player's piece
		if (!(player & mask))
			continue;

		// is the piece also a king
		bool king = (m_kings & mask) != 0;

		// found the piece at mask
		// find its moves

		// compute the forward and backward bitboards
		uint64_t forward, backward;
		if (direction > 0)
		{
			forward = (mask << (width - 1)) | (mask << (width + 1));
			backward = (mask >> (width - 1)) | (mask >> (width + 1));
		}
		else
		{
			backward = (mask << (width - 1)) | (mask << (width + 1));
			forward = (mask >> (width - 1)) | (mask >> (width + 1));
		}

		// compute the 1st degree movements
		uint64_t move = forward;
		if (king)
		{
			direction = 0;
			move |= backward;
		}


		move &= boardmask;  // can only be a legal board position
		move &= ~player;   // cannot move to self

		// compute the pure movement options
		uint64_t movements = move;
		movements &= ~other;  // cannot move to the opponent

		// split the locations
		// there is a maximum of 4 locations to move to
		std::vector<uint64_t> locations = splitbits(movements);
		for (uint64_t to : locations)
		{
			// became king if reached promotion or is already king
			bool becameking = (to & promotion) != 0 || king;
			moves.push_back({ mask, to,{}, becameking });
		}


		// compute jumps
		checkers_compute_jumps(moves, {}, mask, mask, player, other, direction);
	}


	return moves;
}

// performs the given move based on the current player, returns a new board where the move is performed
checkers::board checkers::board::perform_move(checkers::move &move, state turn) const
{
	uint64_t toprow = make_checkers_bitboard(G_CHECKERS_WIDTH - 1, G_CHECKERS_WIDTH - 1);
	uint64_t bottomrow = make_checkers_bitboard(0, 0);

	uint64_t player, other;
	uint64_t promotion;

	if (turn == state::RED)
	{
		player = m_red;
		other = m_black;
		promotion = bottomrow;
	}
	else
	{
		player = m_black;
		other = m_red;
		promotion = toprow;
	}


	// move player piece
	player ^= (move.from | move.to);

	// remove king
	uint64_t king = (m_kings | move.from) ^ move.from;
	// add king if became king
	if (move.king)
		king |= move.to;


	// handle capture chains
	for (auto cap : move.captures)
	{
		other ^= cap;
	}

	if (turn == state::RED)
	{
		return { player, other, king };
	}
	else
	{
		return { other, player, king };
	}
}


checkers::state checkers::board::get_state(state turn) const
{
	// get moves
	std::vector<checkers::move> moves = compute_moves(turn);

	// no moves means the other player wins
	if (moves.size() == 0)
	{
		if (turn == state::RED)
			return state::BLACK;
		return state::RED;
	}

	// we do not deal with draws
	
	return state::NONE;
}
