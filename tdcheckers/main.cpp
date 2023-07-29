
#include <iostream>
#include "checkers.h"

#include "tester.h"
#include "explorer.h"


void playgame(checkers::board board, checkers::state turn)
{
	while (true)
	{
		// display board
		std::cout << "Board\n" << board.repr() << std::endl;

		// check for state
		checkers::state state = board.get_state(turn);

		// if a non-normal state is encountered
		if (state != checkers::state::NONE)
		{
			// announce winner and exit
			switch (state)
			{
			case checkers::state::RED:
			{
				std::cout << "Red won!" << std::endl;
				break;
			}
			case checkers::state::BLACK:
			{
				std::cout << "Black won!" << std::endl;
				break;
			}
			case checkers::state::DRAW:
			{
				std::cout << "Draw!" << std::endl;
				break;
			}
			}

			break;
		}


		// find moves
		std::vector<checkers::move> moves = board.compute_moves(turn);
		for (auto move : moves)
		{
			std::cout << move.str() << ", ";
		}
		std::cout << std::endl;


		// prompt
		std::string prompt;
		if (turn == checkers::state::RED)
		{
			prompt = "red: ";
		}
		else
		{
			prompt = "black: ";
		}

		// ask for a legal move
		int moveindex = -1;
		while (true)
		{
			// get inputs
			std::cout << prompt;
			std::cout.flush();

			std::string text;
			std::cin >> text;

			// check if it is legal
			for (int i = 0; i < moves.size(); ++i)
			{
				if (moves[i].str() == text)
				{
					moveindex = i;
					break;
				}
			}

			// exit if it is legal
			if (moveindex != -1)
				break;

			std::cout << "please enter a legal move" << std::endl;
		}

		checkers::move move = moves[moveindex];

		board = board.perform_move(move, turn);

		if (turn == checkers::state::RED)
			turn = checkers::state::BLACK;
		else
			turn = checkers::state::RED;

		std::cout << std::endl;
	}

}


void test_eval()
{
	checkers::state turn = checkers::state::RED;

	const char initial[] =
		". . . . . . . ."
		". . O . . . . ."
		". X . O . . . ."
		". . . . . . . ."
		". . . O . . . ."
		". . . . O . . ."
		". . . O . . . ."
		". . . . O . . .";
	std::string position(initial);
	checkers::board board{initial};
	
	explorer::optimizer optimizer{board, turn};

	std::cout << board.repr() << std::endl;

	optimizer.compute_score(turn);

	std::cout << "Score: " << optimizer.get_score() << std::endl;

	auto &lines = optimizer.get_lines();
	testing::perform_moves(board, turn, lines);
}



const checkers::move &get_move(const std::vector<checkers::move> &moves, checkers::state turn)
{
	while (true)
	{
		// print prompt
		std::cout << checkers::state_repr(turn) << ": ";
		std::cout.flush();

		// ask for input
		std::string text;
		std::cin >> text;

		for (auto &move : moves)
		{
			if (move.str() == text)
			{
				return move;
			}
		}

		std::cout << "please enter a valid move" << std::endl;
	}
}


void actual_shit()
{
	const char initial[] =
		". x . x . x . x"
		". . x . . . x ."
		". x . . . x . x"
		"o . x . x . . ."
		". . . o . . . o"
		"o . . . o . . ."
		". o . . . . . o"
		"o . o . o . o .";
	std::string position(initial);
	checkers::board board{};
	checkers::state turn = checkers::state::RED;
	checkers::state ai = checkers::state::RED;


	// create optimizer
	explorer::optimizer optimizer{board, ai};

	int i = 0;
	while (true)
	{
		i += 1;
		std::cout << "\n------- Turn " << i << " -------" << std::endl;
		std::cout << board.repr() << std::endl;

		auto moves = board.compute_moves(turn);


		if (turn == ai)
		{
			// compute best move
			optimizer.update_board(board);
			optimizer.compute_score(turn);
			checkers::move best = optimizer.get_move().value();

			// manual override
			std::cout << "Selected " << best.str() << ", override? " << std::endl;

			std::string text;
			std::cin >> text;

			if (text == "yes")
			{ 
				best = get_move(moves, turn);
			}


			// play move
			board = board.perform_move(best, turn);
			turn = checkers::state_flip(turn);
		}
		else
		{
			// print moves
			std::cout << "moves: ";
			for (auto &m : moves)
			{
				std::cout << m.str() << " ";
			}
			std::cout << std::endl;

			checkers::move move = get_move(moves, turn);

			// play move
			board = board.perform_move(move, turn);
			turn = checkers::state_flip(turn);
		}
	}












}


int main()
{
	
	actual_shit();
	
	//testing::explore_moves({}, checkers::state::RED);

	return 0;

	checkers::state turn = checkers::state::BLACK;
	
	const char initial[] =
		". . . x . x . x"
		". . x . x . x ."
		". x . x . x . x"
		"x . o . o . . ."
		". x . . . . . ."
		". . . . . . o ."
		". o . o . o . o"
		"o . o . o . o .";
	std::string position(initial);
	checkers::board board{position};
	//testing::play_itself(board, checkers::state::RED);
	//testing::explore_moves(board, checkers::state::RED);
	testing::analyze(board, checkers::state::RED);
	return 0;


	/*test_eval();
	return 0;*/



	//testing::random_play();
	//return 0;

	//const char initial[] =
	//	". O . . . . . ."
	//	". . . . . . . ."
	//	". . . . . . . ."
	//	". . . . . . . ."
	//	". . . . . . . ."
	//	". . x . . . . ."
	//	". . . o . . . ."
	//	". . . . . . . .";
	//std::string position(initial);
	//checkers::board board{initial};
	//testing::explore_moves(board, checkers::state::RED);
	//return 0;

	//playgame(board, checkers::state::RED);
	return 0;
}