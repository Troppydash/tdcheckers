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
	
	//playgame(board, turn);

	explorer::optimizer optimizer{board, turn};

	std::cout << board.repr() << std::endl;

	optimizer.compute_score(turn);

	std::cout << "Score: " << optimizer.get_score() << std::endl;

	auto &lines = optimizer.get_lines();
	testing::perform_moves(board, turn, lines);
}

int main()
{
	checkers::state turn = checkers::state::BLACK;

	const char initial[] =
		". x . . . . . ."
		". . x . . . . ."
		". x . x . . . ."
		". . . . . . . ."
		". . . . . . . ."
		". . . . . . . ."
		". o . o . o . o"
		"o . o . o . o .";
	std::string position(initial);
	checkers::board board{initial};
	testing::play_against(position, turn, checkers::state::RED);
	return 0;


	test_eval();
	return 0;



	//testing::random_play();
	//return 0;

	//const char initial[] =
	//	". O . . . . . ."
	//	". . . . . . . ."
	//	". X . O . . . ."
	//	". . O . . . . ."
	//	". . . O . . . ."
	//	". . . . O . . ."
	//	". . . O . . . ."
	//	". . . . O . . .";
	//std::string position(initial);
	//checkers::board board{initial};
	//testing::explore_moves(board, checkers::state::RED);
	//return 0;

	//playgame(board, checkers::state::RED);
	return 0;
}