#pragma once

#include "GameBoyDefs.h"
#include "GameBoyMBC.h"

namespace GameBoy
{

/*
0000-3FFF - ROM Bank 00 (Чтение)

4000-7FFF - ROM Bank 01-0F (Чтение)

A000-A1FF - 512x4bits RAM, встроенный в MBC2 чип (Чтение/Запись)

0000-1FFF - RAM вкл. (Чтение)

2000-3FFF - ROM Bank Number (Запись)
*/
class MBC2 : public MBC
{
public:
	MBC2(BYTE *ROM, DWORD ROMSize, BYTE *RAMBanks, DWORD RAMSize) : MBC(ROM, ROMSize, RAMBanks, RAMSize)
	{
		ROMOffset = ROMBankSize;
		RAMOffset = 0;
	}

	virtual void Write(WORD addr, BYTE value)				//запись
	{
		switch (addr & 0xF000)
		{
		//переключение ROM bank
		case 0x2000:
		case 0x3000:
			ROMOffset = value & 0xF;
			ROMOffset %= ROMSize;

			if (ROMOffset == 0)
			{
				ROMOffset = 1;
			}

			ROMOffset *= ROMBankSize;
			break;

		//RAM bank 0
		case 0xA000:
		case 0xB000:
			RAMBanks[addr - 0xA000] = value & 0xF;
			break;
		}
	}

	virtual BYTE Read(WORD addr)							//чтение
	{
		switch (addr & 0xF000)
		{
		//ROM bank 0
		case 0x0000:
		case 0x1000:
		case 0x2000:
		case 0x3000:
			return ROM[addr];

		//ROM bank 1
		case 0x4000:
		case 0x5000:
		case 0x6000:
		case 0x7000:
			return ROM[ROMOffset + (addr - 0x4000)];

		//RAM bank 0
		case 0xA000:
		case 0xB000:
			return RAMBanks[addr - 0xA000] & 0xF;
		}

		return 0xFF;										//содержимое не определено
	}
};

}