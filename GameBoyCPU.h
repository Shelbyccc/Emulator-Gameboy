#pragma once

#include "GameBoyDefs.h"

namespace GameBoy
{

	class Memory;
	class GPU;
	class DividerTimer;
	class Joypad;
	class Interrupts;

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

		CPU(Memory &MMU, GPU &GPU, DividerTimer &DIV, Joypad &joypad, Interrupts &INT);

		void Step();

		void Reset();
		void EmulateBIOS();				//������������� ��������� ����������

		friend class Interrupts;

	private:

		BYTE MemoryRead(WORD addr);		//������ ����� �� ������
		WORD MemoryReadWord(WORD addr);	//������ ����� �� ������
		void MemoryWrite(WORD addr, BYTE value);	//������ ����� � ������
		void MemoryWriteWord(WORD addr, WORD value);//������ ����� � ������

		void INTJump(WORD address);	//������� �� ��������� ����� ����������

		void StackPushByte(BYTE value);	//�������� ����� � ����
		void StackPushWord(WORD value);	//�������� ����� � ����
		BYTE StackPopByte();			//�������� ����� �� �����
		WORD StackPopWord();			//�������� ����� �� �����

		//CPU ��������
		BYTE A;
		BYTE F;//�������� �������
		WordRegister BC;
		WordRegister DE;
		WordRegister HL;
		WORD PC;//������� ������ PC. 
		WORD SP;//��������� ����� SP. 
		BYTE IME;//Interrupt master enable register

		bool Halted;	//���� ����, ��� ��������� ���������
		bool HaltBug;	//��� �������������� ����

		BYTE DIDelay;	//�������� ��� ������� ����������
		BYTE EIDelay;	//�������� ��� ���������� ����������

		Memory &MMU;
		GPU &GPU;
		DividerTimer &DIV;
		Joypad &Joypad;
		Interrupts &INT;
	};

}