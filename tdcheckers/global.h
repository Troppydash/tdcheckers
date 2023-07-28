#pragma once

#include "checkers.h"

#define G_BOARDMASKS_SIZE (8*8)

namespace global
{
	struct boardmask
	{
		constexpr boardmask() : masks()
		{
			int n = 0;
			for (int i = 0; i < G_CHECKERS_SIZE; ++i)
			{
				int row = i / G_CHECKERS_WIDTH;
				int col = i % G_CHECKERS_WIDTH;
				int offset = 1 - (row % 2);

				// skip 
				if ((col - offset) % 2 != 0)
					continue;

				uint64_t mask = 1ull << i;
				masks[n++] = mask;
			}
		}

		uint64_t masks[G_BOARDMASKS_SIZE];
		size_t size = G_BOARDMASKS_SIZE;
	};
}