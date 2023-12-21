#pragma once

#include <string>

namespace uci
{
	/**
	 * @brief Implements the universal checkers interface
	*/
	class uci
	{
	public:
		uci();

		// handles a given command
		void handle(const std::string &command);

	protected:

	};


	// attaches an uci interface to the stdin and stdout
	void attach(uci &interface);
}


