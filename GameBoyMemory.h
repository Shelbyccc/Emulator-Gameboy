#pragma once

#include "GameBoyDefs.h"

namespace GameBoy
{
class Memory											//������
{
public:
	void Write(WORD addr, BYTE value);					//����� ����������� ������
	BYTE Read(WORD addr);								//� ������
};
}