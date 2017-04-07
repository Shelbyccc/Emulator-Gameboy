#pragma once

#include "GameBoyDefs.h"
#include <stdio.h>

namespace GameBoy
{

const int ROMBankSize = 0x4000;
const int RAMBankSize = 0x2000;

class MBC														//базовый класс
{
public:
	virtual void Write(WORD addr, BYTE value) = 0;
	virtual BYTE Read(WORD addr) = 0;
	
	virtual bool SaveRAM(const char *path, DWORD RAMSize);		//Сохранение и загрузка будут реализованы позже

	virtual bool LoadRAM(const char *path, DWORD RAMSize);

protected:
	MBC(BYTE *ROM, DWORD ROMSize, BYTE *RAMBanks, DWORD RAMSize) : ROM(ROM), ROMSize(ROMSize), RAMBanks(RAMBanks), RAMSize(RAMSize) {}

	BYTE *ROM;				//Здесь у нас находится весь образ игры.
	BYTE *RAMBanks;			//Здесь находится оперативная память картриджа.

	DWORD ROMOffset;		//Это смещения, которые указывают на текущий банк памяти.
	DWORD RAMOffset;		

	DWORD ROMSize;			
	DWORD RAMSize;			
};

}
