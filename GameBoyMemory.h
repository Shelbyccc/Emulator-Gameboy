#pragma once

#include "GameBoyDefs.h"
#include "GameBoyROMInfo.h"

namespace GameBoy
{

	class GPU;
	class DividerTimer;
	class TIMATimer;
	class Joypad;
	class Interrupts;
	class MBC;

	const int WRAMBankSize = 0x1000;

	class Memory
	{
	public:
		/*
		�������� � "I/O ports" ����� ���
		*/
		enum IOPortsEnum
		{
			IOPORT_P1 = 0x00,	//������� ��� ������ ���������� � ���������� (R/W)
			IOPORT_DIV = 0x04,	//������� �������� �������. ���������� ��� ��������� (R/W)
			IOPORT_IF = 0x0F,	//���� ���������� (R/W)
			IOPORT_LCDC = 0x40,	//LCD ���������� (R/W)
			IOPORT_STAT = 0x41,	//LCDC ������ (R/W)
			IOPORT_SCY = 0x42,	//������������ ���������� Y (R/W)
			IOPORT_SCX = 0x43,	//�������������� ���������� X (R/W)
			IOPORT_LY = 0x44,	//LCDC Y-��������� ������. ������ �������� � ��������� �������� (R)
			IOPORT_LYC = 0x45,	//��� ��������� � LY (R/W)
			IOPORT_DMA = 0x46,	//DMA �������� � �������� ����� (W)
			IOPORT_BGP = 0x47,	//���������� � ������� (R/W)
			IOPORT_OBP0 = 0x48,	//������ ������� �������� �1 (R/W)
			IOPORT_OBP1 = 0x49,	//������ ������� �������� �2 (R/W)
			IOPORT_WY = 0x4A,	//������� ���� �� Y (R/W)
			IOPORT_WX = 0x4B,	//������� ���� �� X (R/W)
			IOPORT_VBK = 0x4F,	//������� ���� �����������
			IOPORT_SVBK = 0x70	//WRAM ����
		};

		Memory(GPU &GPU, DividerTimer &DIV, Joypad &joypad, Interrupts &INT);
		~Memory();

		const ROMInfo* LoadROM(const wchar_t* ROMPath);		//�������� ���
		bool IsROMLoaded() { return ROMLoaded; }			
		void Reset(bool clearROM = false);

		void EmulateBIOS();

		void Write(WORD addr, BYTE value);
		BYTE Read(WORD addr);

		bool SaveRAM();										//���������� ���
		bool LoadRAM();										//�������� ���

	private:
		bool InBIOS;
		static BYTE BIOS[0x100];							//������� ������� ����� ������ (256 ����)

		/*
		������ ������				��������� �����	�������� �����
		ROM bank 0					0x0000			0x3FFF
		Switchable ROM bank			0x4000			0x7FFF
		Video RAM					0x8000			0x9FFF
		Switchable RAM bank			0xA000			0xBFFF
		Internal RAM 1				0xC000			0xDFFF
		Echo of Internal RAM 1		0xE000			0xFDFF
		OAM							0xFE00			0xFE9F
		�� ������������				0xFEA0			0xFEFF
		I/O ports					0xFF00			0xFF4B
		�� ������������				0xFF4C			0xFF7F
		Internal RAM 2				0xFF80			0xFFFE
		Interrupt enable register	0xFFFF			0xFFFF
		*/

		BYTE* ROM;								//ROM banks 0, 1			
		BYTE* RAMBanks;							//Switchable RAM bank		
		BYTE WRAM[WRAMBankSize * 2];			//Internal RAM 1			
		BYTE HRAM[0x7F];						//Internal RAM 2		
		//��������� ������� ������ ����������� � ������ ������ ���������.

		DWORD WRAMOffset;	//��������� �� ������� ����� ����� WRAM

		MBC *MBC;			//���������� ������ ������

		//��������
		bool ROMLoaded;						//���� ����, ��� �������� ����������
		ROMInfo LoadedROMInfo;				//���������� � ���������
		static BYTE NintendoGraphic[48];	//������� Nintendo

		//���������� �����
		GPU &GPU;			//��������������
		Joypad &Joypad;		//����������
		DividerTimer &DIV;	//������� DIV
		Interrupts &INT;	//����������
	};

}