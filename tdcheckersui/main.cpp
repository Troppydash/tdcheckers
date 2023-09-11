#include <glad/glad.h>


#include <iostream>
//#include "tdcheckers/tester.h"
//#include "tdcheckers/checkers.h"

#include "tdwindow.h"
#include "board.h"

int main()
{
	td::window window({ 1024, 768, "Checkers GUI"});

	// setup
	gui::checkers_board board;
	window.include(&board);

	window.start();

	board.restart_game();

	// render loop
	window.main();

	// cleanup
	window.end();
	
	return 0;
}