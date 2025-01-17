#pragma once

#include "GameBoyDefs.h"

namespace GameBoy
{

/*
0100h - 0103h. ����� ����� � ��������� ���������. ���������� ���������� � ��� ��������� ���������� � ������ 0100h. �� ���� ������� ���������� ������� NOP � ������� �������� �� ������ ��������� JP. 
���� �������� �������� ��������� �������:

0100 00 NOP 
0101 �� �� XX JP xxxxh 
; ���� - ����� �������.

0104h - 0133h. ����� �������� ����������� ��������� ����� ����� NINTENDO, ������������ �� ������ � ������ ����. ��������� ���� �� ������ ����� � ���� ������� ������ �������� � ������������������� ���������. 
���� ��������� ���������� ������ �������:

0104 �� ED 66 66 �� 0D 00 0� 
010� 03 73 00 83 00 �� 00 0D 
0114 00 08 11 1F 88 89 00 0� 
011� DC �� 6� �6 DD DD D9 99 
0124 �� �� 67 63 6� 0� �� �� 
012� DD DC 99 9F �� �9 33 3�

0134h - 0142h. �������� ����, ���������� ���������� ����������� ������� � ��������� ASCII. ��������� ������ ����������� ����� 00h. 
014 3h. ���� ����� ���������� ��� 80h, ��������� ������������� ��� ������� ������� COLOR GAME BOY. 
0144h, 0145h. ������������ ���. ���� ���������� ������ � ������� 014Bh �� ����� 33h, �� �� ���� ������� �������� ����. 
0146h. ��� �������: 00h ��� GAME BOY, 03h ��� SUPER GAME BOY. ���� ����� ������� ��� 00h, �������������� ������� ������� ������� SUPER GAME BOY �� ������������. 
0147h. ���������� � ��������� ���������. �������� ��������� ��������: 00 - ������� ������ ���; 01h - ������� ��� � ���������� ���1; 02h - ������� ���, ���������� ���1 � ���;
03h - ������� ���, ���������� MBC1 ��� � �������; 
05h - ������� ��� � ���������� ���2; 
0 6h - ������� ���, ���������� ���2 � �������; 08h - ������� ��� � ���; 
09h - ������� ���, ��� � �������; 0Bh - ������� ��� � ���������� ���01; 0�h - ������� ���, ���������� ���01 � ���; 
0Dh - ������� ���, ���������� ���01, ��� � �������; 
0Fh - ������� ���, ���������� ����, ������) � �������; 
10h - ������� ���, ���������� ����, ���, ������ � �������; 
1 lh - ������� ��� � ���������� ����; 
12h - ������� ���, ���������� ���� � ���; 
13h - ������� ���, ���������� ����, ��� � �������; 
19h - ������� ��� � ���������� ���5; 
1Ah - ������� ���, ���������� ���5 � ���; 
1Bh - ������� ���, ���������� ���5, ���; � �������; 
1Ch - ������� ���, ���������� ���5 � ����� RUMBLE; 
1Dh - ������� ���, ���������� ���5, ����� RUMBLE � ���; 
1Eh - ������� ���, ���������� ���5, ����� RUMBLE, ��� � �������; 
1Fh - ������ ��� GAME BOY; 
FDh - �������� BANDAI ����5 (��������); 
FEh - ���������� HUDSON HUC3; 
FFh - ���������� HUDSON HUC1.

0148h. ���������� �� ������ �������������� � ��������� ���: 
00h - 32 �� (2 �����); 
01h - 64 �� (4 �����); 
02h - 128 �� (8 ������); 
03h - 256 �� (16 ������); 
04h - 512 �� (32 �����); 
05h- 1 �� (64 �����); 
06h- 2 �� (128 ������); 
52h - 1,1 �� (72 �����); 
53h - 1,2 �� (80 ������); 
54h - 1,5 �� (96 ������).

0149h. ���������� �� ������ �������������� � ��������� ���: 
00h - ��� �� �����������; 
01h - 2 �� (1 ����); 
02h - 8 �� (1 ����); 
03h - 32 �� (4 �����); 
04h - 128 �� (16 ������).

014Ah. ��� ������ ��������������� ���������: 
00h - ������; 
01h - ������ ������.

014Bh. ������������ ��� �������������. ��� ������������� �������������� ������� ������� SUPER GAME BOY � ���� ������� ������ ���� ������� ��� 33h. 
33h - ������������ ��� ������ � ������ � �������� 0144h, 0145h. 
79h - Accolade; 
A4h - Konami.

014Ch. ��� ���������� ���. ������ � ���� ����� �������� �������� 00h. 
014Dh. �������������� ��� ����� ������. ���������, ������������� �� ���������� ��� �������, ��������� ����� ����� ������, ������� ��������� � ��� ��������� �� ������� 0134h - 014Dh.
����� � ���������� ������������ 25. ���������� ���������, ���������� � ���������, ���������� ������ � ��� ������, ���� ������� ���� ���������� ����� ����� 0. 
014Eh, 014Fh. ����������� ����� ����������� ���������, ������� ����������� ����� ����������������� �������� ���� ������, ���������� � ��� ��������� (����� ������ � ����������� ������).
������� GAME BOY ���������� ��� ��������. ��� ���������� ������ ������� SUPER GAME BOY ����� ������ ��������� ���������� ����������� �����. 
*/
	struct ROMInfo
	{
	public:
		enum CartridgeTypesEnum
		{
			CART_ROM_ONLY = 0x00,
			CART_ROM_MBC1 = 0x01,
			CART_ROM_MBC1_RAM = 0x02,
			CART_ROM_MBC1_RAM_BATT = 0x03,
			CART_ROM_MBC2 = 0x05,
			CART_ROM_MBC2_BATT = 0x06,
			CART_ROM_RAM = 0x08,
			CART_ROM_RAM_BATT = 0x09,
			CART_ROM_MMM01 = 0x0B,
			CART_ROM_MMM01_SRAM = 0x0C,
			CART_ROM_MMM01_SRAM_BATT = 0x0D,
			CART_ROM_MBC3_TIMER_BATT = 0x0F,
			CART_ROM_MBC3_TIMER_RAM_BATT = 0x10,
			CART_ROM_MBC3 = 0x11,
			CART_ROM_MBC3_RAM = 0x12,
			CART_ROM_MBC3_RAM_BATT = 0x13,
			CART_ROM_MBC5 = 0x19,
			CART_ROM_MBC5_RAM = 0x1A,
			CART_ROM_MBC5_RAM_BATT = 0x1B,
			CART_ROM_MBC5_RUMBLE = 0x1C,
			CART_ROM_MBC5_RUMBLE_SRAM = 0x1D,
			CART_ROM_MBC5_RUMBLE_SRAM_BATT = 0x1E,
			CART_POCKET_CAMERA = 0x1F,
			CART_BANDAI_TAMA5 = 0xFD,
			CART_HUDSON_HUC3 = 0xFE,
			CART_HUDSON_HUC1 = 0xFF,
			CART_UNKNOWN = 0x100
		};

		enum MMCTypesEnum
		{
			MMC_ROMONLY = 0x00,
			MMC_MBC1 = 0x01,
			MMC_MBC2 = 0x02,
			MMC_MBC3 = 0x03,
			MMC_MBC5 = 0x04,
			MMC_MMM01 = 0x05,
			MMC_UNKNOWN = 0x06
		};

		enum ROMSizesEnum
		{
			ROMSIZE_2BANK = 0x0,
			ROMSIZE_4BANK = 0x1,
			ROMSIZE_8BANK = 0x2,
			ROMSIZE_16BANK = 0x3,
			ROMSIZE_32BANK = 0x4,
			ROMSIZE_64BANK = 0x5,
			ROMSIZE_128BANK = 0x6,
			ROMSIZE_72BANK = 0x52,
			ROMSIZE_80BANK = 0x53,
			ROMSIZE_96BANK = 0x54
		};

		enum RAMSizesEnum
		{
			RAMSIZE_NONE = 0x0,
			RAMSIZE_HALFBANK = 0x1,
			RAMSIZE_1BANK = 0x2,
			RAMSIZE_4BANK = 0x3,
			RAMSIZE_16BANK = 0x4
		};

		wchar_t ROMFile[255];				//�������� �����
		char gameTitle[17];					//���������			
		CartridgeTypesEnum cartType;		//��� ���������
		MMCTypesEnum MMCType;				//��� ����������� ������
		int ROMSize;						//������ ���
		int RAMSize;						//������ ���

		ROMInfo();
		void ReadROMInfo(const BYTE* ROMBytes);

		const char* CartTypeToString(CartridgeTypesEnum type) const { return CartridgeTypes[type]; }

	private:
		const char* CartridgeTypes[0x101];	//Cartridge type # -> Cartridge type string table
		int ROMSizes[0xFF];					//ROM size # -> ROM size value table
		int RAMSizes[0xFF];					//RAM size # -> RAM size value table
	};

}