#include "tester.h"

#include <vector>
#include <unordered_set>
#include <random>
#include "explorer.h"

void testing::explore_moves(checkers::board position, checkers::state turn)
{
	std::vector<checkers::board> positions{position};
	//std::unordered_set<checkers::board, checkers::board::hash_function> positions{position};

	int depth = 0;
	while (true)
	{
		size_t actions = 0;
		std::vector<checkers::board> nextpositions;
		//std::unordered_set<checkers::board, checkers::board::hash_function> nextpositions;
		for (auto &board : positions)
		{
			std::vector<checkers::move> moves = position.compute_moves(turn);
			actions += moves.size();

			for (auto &m : moves)
				nextpositions.push_back(board.perform_move(m, turn));
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

void testing::random_play()
{
	checkers::board board;
	checkers::state turn = checkers::state::RED;

	// rng
	std::random_device dev;
	std::mt19937 rng(dev());
	std::uniform_real_distribution<> dist(0.0f, 0.9999f);

	while (true)
	{
		std::cout << board.repr() << std::endl;

		checkers::state state = board.get_state(turn);
		if (state != checkers::state::NONE)
		{
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


		// get moves
		std::vector<checkers::move> moves = board.compute_moves(turn);

		for (auto &move : moves)
		{
			std::cout << move.str() << " ";
		}
		std::cout << std::endl;

		// randomly select a move
		double rand = dist(rng);
		int choice = floor(rand * moves.size());
		checkers::move &move = moves[choice];

		// play move
		std::cout << checkers::state_repr(turn) << " played: " << move.str() << std::endl;

		// switch turns and play move
		board = board.perform_move(move, turn);
		if (turn == checkers::state::RED)
			turn = checkers::state::BLACK;
		else
			turn = checkers::state::RED;
	}

}

void testing::perform_moves(checkers::board position, checkers::state turn, const std::vector<checkers::move> &moves)
{
	for (auto &move : moves)
	{

		std::cout << position.repr() << std::endl;
		std::cout << checkers::state_repr(turn) << "'s move" << std::endl;

		position = position.perform_move(move, turn);
		turn = checkers::state_flip(turn);
	}

	std::cout << position.repr() << std::endl;

}

void testing::play_against(checkers::board position, checkers::state player, checkers::state turn)
{
	explorer::optimizer optimizer{position, checkers::state_flip(player)};

	while (true)
	{
		std::cout << position.repr() << std::endl;

		if (position.get_state(turn) != checkers::state::NONE)
		{
			std::cout << "Finished" << std::endl;
			break;
		}

		if (player == turn)
		{
			auto moves = position.compute_moves(turn);

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
			position = position.perform_move(move, turn);
			turn = checkers::state_flip(turn);
		}

		else
		{
			optimizer.update_board(position);

			optimizer.compute_score(turn);

			std::cout << "Score = " << optimizer.get_score() << std::endl;

			checkers::move best = optimizer.get_move().value();

			// perform move
			position = position.perform_move(best, turn);
			turn = checkers::state_flip(turn);
		}
	}
	

}

void testing::play_itself(checkers::board position, checkers::state turn)
{
	checkers::board firstboard, secondboard;
	firstboard.copy(position);
	secondboard.copy(position);

	explorer::optimizer first{firstboard, turn};
	explorer::optimizer second{secondboard, checkers::state_flip(turn)};

	bool isfirst = true;

	while (true)
	{
		std::cout << position.repr() << std::endl;

		if (position.get_state(turn) != checkers::state::NONE)
		{
			std::cout << "Finished" << std::endl;
			break;
		}

		if (isfirst)
		{
			first.update_board(position);
			first.compute_score(turn);

			std::cout << "Score1 = " << first.get_score() << std::endl;

			checkers::move best = first.get_move().value();

			// perform move
			position = position.perform_move(best, turn);
			turn = checkers::state_flip(turn);
		}

		else
		{
			second.update_board(position);
			second.compute_score(turn);

			std::cout << "Score2 = " << second.get_score() << std::endl;

			checkers::move best = second.get_move().value();

			// perform move
			position = position.perform_move(best, turn);
			turn = checkers::state_flip(turn);
		}

		isfirst = !isfirst;
	}
}

void testing::analyze(checkers::board position, checkers::state turn)
{
	std::cout << position.repr() << std::endl;

	explorer::optimizer optimizer{position, turn};

	optimizer.compute_score(turn);

	std::cout << "Score is " << optimizer.get_score() << std::endl;
	std::cout << "Best is " << optimizer.get_move().value().str() << std::endl;
}
