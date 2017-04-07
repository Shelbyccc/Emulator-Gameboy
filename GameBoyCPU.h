#pragma once

#include "GameBoyDefs.h"

namespace GameBoy
{
class CPU
{
public:
	union WordRegister
	{
		struct
		{
			BYTE L;
			BYTE H;
		} bytes;
		WORD word;
	};
	void Step();
};

}