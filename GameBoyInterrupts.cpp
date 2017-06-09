#include "GameBoyInterrupts.h"
#include "GameBoyCPU.h"

/*
����������			���������	��������� �����
V-Blank				1			$0040 (�������� �������������)
LCDC Status			2			$0048 - ������ 0, 1, 2, LYC=LY ���������� (�� ������) (��������������)
Timer Overflow		3			$0050 (������������ �������)
Serial Transfer		4			$0058 - ����� �������� ����� ��������� (���������������� ���������)
Hi-Lo of P10-P13	5			$0060 (����������)
*/
void GameBoy::Interrupts::Step(GameBoy::CPU &CPU)
{
	//�� �� ����� ��������� IME �����
	if ((IE & IF) & 0x1F)	//1Fh = 00011111b
	{
		//���������� ��������� HALT, ���� ���� ��� �� ����� ������������� ��-�� IME
		if (CPU.Halted)
		{
			CPU.Halted = false;
			CPU.PC++;
		}

		//������ �� ��������� IME �� ������������ ����������. ������� � ������� ���������� �������� 20 ������
		if (CPU.IME)
		{
			CPU.IME = 0;	//������������ ���� IME, � ����������� ����� ����� �������� �� ����������. 

			CPU.StackPushWord(CPU.PC);		//��������� �������� ������ (PC) ����������� � �����.

			if ((IF & INTERRUPT_VBLANK) && (IE & INTERRUPT_VBLANK))//V-blank interrupt (�������� �������������)
			{
				IF = 0;	//������� ������ �� ����������

				CPU.INTJump(0x40);	//���������� ���������� �� ��������� ����� ��������� ������������ ���������������� ����������.
			}
			else if ((IF & INTERRUPT_LCDC) && (IE & INTERRUPT_LCDC))//LCDC status (��������������)
			{
				IF = 0;

				CPU.INTJump(0x48);
			}
			else if ((IF & INTERRUPT_TIMA) && (IE & INTERRUPT_TIMA))//Timer overflow (������������ �������)
			{
				IF = 0;

				CPU.INTJump(0x50);
			}
			else if ((IF & INTERRUPT_SERIAL) && (IE & INTERRUPT_SERIAL))//Serial transfer (���������� ����������������� ����������)
			{
				IF = 0;

				CPU.INTJump(0x58);
			}
			else if ((IF & INTERRUPT_JOYPAD) && (IE & INTERRUPT_JOYPAD))//Joypad (����������)
			{
				IF = 0;

				CPU.INTJump(0x60);
			}
		}
	}
}