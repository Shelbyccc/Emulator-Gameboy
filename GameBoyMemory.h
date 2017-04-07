#pragma once

#include "GameBoyDefs.h"

namespace GameBoy
{
class Memory											//память
{
public:
	void Write(WORD addr, BYTE value);					//нужно реализовать Запись
	BYTE Read(WORD addr);								//и Чтение
};
}