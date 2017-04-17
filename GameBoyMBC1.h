#pragma once

#include "GameBoyDefs.h"
#include "GameBoyMBC.h"

namespace GameBoy
{

/*
0000-3FFF - ROM Bank 00 (������)

4000-7FFF - ROM Bank 01-7F (������)

A000-BFFF - RAM Bank 00-03, ���� ������� (������/������)

0000-1FFF - RAM ���. (������)

2000-3FFF - ROM Bank Number (������)

4000-5FFF - RAM Bank Number - or - ������� ���� of ROM Bank Number (������)

6000-7FFF - ROM/RAM ����� ������ (������)
*/
class MBC1 : public MBC
{
public:
	enum MBC1ModesEnum				//ROM/RAM ������
	{
		MBC1MODE_16_8 = 0,			//16Mbit ROM/8Kbyte RAM
		MBC1MODE_4_32 = 1			//4Mbit ROM/32KByte RAM
	};

	MBC1(BYTE *ROM, DWORD ROMSize, BYTE *RAMBanks, DWORD RAMSize) : MBC(ROM, ROMSize, RAMBanks, RAMSize)
	{
		RAMEnabled = false;			//RAM ���������
		Mode = MBC1MODE_16_8;		//����� 16/8
		RAMOffset = 0;				//�������� RAM �������
		ROMOffset = ROMBankSize;	//�������� ROM = ������� �����
	}

	virtual void Write(WORD addr, BYTE value)			//������
	{
		switch (addr & 0xF000)
		{
		//RAM ���/����
		case 0x0000:
		case 0x1000:
			RAMEnabled = (value & 0x0F) == 0x0A;		//RAM ���
			break;

		//��������� ROM bank �� ������
		case 0x2000:
		case 0x3000:
			//��������� 5 ������� ��� �������� (0x1F) ROMOffset ��� ������������ 2 MSB (0x60)
			ROMOffset = (((ROMOffset / ROMBankSize) & 0x60) | (value & 0x1F));			//�������� �����
			ROMOffset %= ROMSize;

			if (ROMOffset == 0x00) ROMOffset = 0x01;									//��� ������ �������� ����� �������������� �� ���������
			else if (ROMOffset == 0x20) ROMOffset = 0x21;										
			else if (ROMOffset == 0x40) ROMOffset = 0x41;									
			else if (ROMOffset == 0x60) ROMOffset = 0x61;

			ROMOffset *= ROMBankSize;													//�������� ��������
			break;

		//������������ RAM bank/��������� 2 ������� ����� ROM bank ������
		case 0x4000:
		case 0x5000:
			if (Mode == MBC1MODE_16_8)
			{
				ROMOffset = ((ROMOffset / ROMBankSize) & 0x1F) | ((value & 0x3) << 5);	//���������� 2 ������� ����
				ROMOffset %= ROMSize;

				if (ROMOffset == 0x00) ROMOffset = 0x01;								//��� ������ �������� ����� �������������� �� ���������
				else if (ROMOffset == 0x20) ROMOffset = 0x21;
				else if (ROMOffset == 0x40) ROMOffset = 0x41;
				else if (ROMOffset == 0x60) ROMOffset = 0x61;

				ROMOffset *= ROMBankSize;												//�������� ��������
			}
			else																		//�����
			{
				RAMOffset = value & 0x3;												//���������� ����� RAM bank			
				RAMOffset %= RAMSize;
				RAMOffset *= RAMBankSize;
			}
			break;

		//MBC1 ������������ ������ 
		case 0x6000:
		case 0x7000:
			if ((value & 0x1) == 0)
			{
				Mode = MBC1MODE_16_8;
			}
			else
			{
				Mode = MBC1MODE_4_32;
			}
			break;

		//������������� RAM bank(������ � ���� ��������, ���� �� ����������)
		case 0xA000:
		case 0xB000:
			if (RAMEnabled)
			{
				if (Mode == MBC1MODE_16_8)
				{
					RAMBanks[addr - 0xA000] = value;
				}
				else
				{
					RAMBanks[RAMOffset + (addr - 0xA000)] = value;
				}
			}
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
			return ROM[addr];

		//ROM bank 1
		case 0x4000:
		case 0x5000:
		case 0x6000:
		case 0x7000:
			return ROM[ROMOffset + (addr - 0x4000)];

		//Switchable RAM bank
		case 0xA000:
		case 0xB000:
			if (RAMEnabled)
			{
				if (Mode == MBC1MODE_16_8)
				{
					return RAMBanks[addr - 0xA000];
				}
				else
				{
					return RAMBanks[RAMOffset + (addr - 0xA000)];
				}
			}
		}

		return 0xFF;																	//���������� �� ����������
	}

private:
	bool RAMEnabled;
	MBC1ModesEnum Mode;	//����� ������ MBC1
};

}