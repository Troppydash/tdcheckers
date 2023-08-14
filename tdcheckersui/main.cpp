#include <iostream>
#include "tdcheckers/tester.h"
#include "tdcheckers/checkers.h"

// TODO: Finish OPENGL Setup
#include <glad/glad.h>

int main()
{

	testing::play_itself({}, checkers::state::RED);
	
	return 0;
}