#pragma once

#include "GameBoyDefs.h"
#include "GameBoyMBC.h"

namespace GameBoy
{

	/*
	��������� ���� ������� �� ����� 32 ����� ROM �� ������� ���� MBC ��� ROM-�����.
	��� ��������������� �������������� � ������� � 0000-7FFFh. ��������, � A000-BFFF ����� ���������� �� 8 ����� ���,
	*/
	class MBC_ROMOnly : public MBC
	{
	public:
		MBC_ROMOnly(BYTE *ROM, DWORD ROMSize, BYTE *RAMBanks, DWORD RAMSize) : MBC(ROM, ROMSize, RAMBanks, RAMSize) {}

		virtual void Write(WORD addr, BYTE value)
		{
			switch (addr & 0xF000)
			{
				//ROM bank 0
			case 0x0000:
			case 0x1000:
			case 0x2000:
			case 0x3000:
				//Switchable ROM bank
			case 0x4000:
			case 0x5000:
			case 0x6000:
			case 0x7000:
				//ROM is read-only
				break;

				//Switchable RAM bank
			case 0xA000:
			case 0xB000:
				RAMBanks[addr - 0xA000] = value;
				break;
			}
		}

		virtual BYTE Read(WORD addr)
		{
			switch (addr & 0xF000)
			{
				//ROM bank 0
			case 0x0000:
			case 0x1000:
			case 0x2000:
			case 0x3000:
				//Switchable ROM bank
			case 0x4000:
			case 0x5000:
			case 0x6000:
			case 0x7000:
				return ROM[addr];

				//Switchable RAM bank
			case 0xA000:
			case 0xB000:
				return RAMBanks[addr - 0xA000];

			default:
				return 0xFF;
			}
		}
	};

}