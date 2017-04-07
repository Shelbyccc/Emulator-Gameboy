#pragma once

#include "GameBoyDefs.h"
#include "GameBoyMBC.h"

namespace GameBoy
{

/*
0000-3FFF - ROM Bank 00 (Read Only)

4000-7FFF - ROM Bank 01-7F (Read Only)

A000-BFFF - RAM Bank 00-03, if any (Read/Write)

0000-1FFF - RAM Enable (Write Only)

2000-3FFF - ROM Bank Number (Write Only)

4000-5FFF - RAM Bank Number - or - Upper Bits of ROM Bank Number (Write Only)

6000-7FFF - ROM/RAM Mode Select (Write Only)
*/
class MBC1 : public MBC
{
public:

	MBC1(BYTE *ROM, DWORD ROMSize, BYTE *RAMBanks, DWORD RAMSize) : MBC(ROM, ROMSize, RAMBanks, RAMSize)
	{
	}

	virtual void Write(WORD addr, BYTE value)
	{
		switch (addr & 0xF000)
		{
		//RAM enable
		case 0x0000:
		case 0x1000:
			break;
		//ROM Bank Number
		case 0x2000:
		case 0x3000:
			break;
		//RAM Bank Number - or - Upper Bits of ROM Bank Number
		case 0x4000:
		case 0x5000:
			break;
		//RAM Bank 00-03
		case 0xA000:
		case 0xB000:
			break;
		}
	}

	virtual BYTE Read(WORD addr)
	{
		switch (addr & 0xF000)
		{
		//ROM bank 00
		case 0x0000:
		case 0x1000:
		case 0x2000:
		case 0x3000:
			break;
		//ROM Bank 01-7F
		case 0x4000:
		case 0x5000:
		case 0x6000:
		case 0x7000:
			break;
		//RAM Bank 00-03, if any
		case 0xA000:
		case 0xB000:
			break;
		}
	}
};

}