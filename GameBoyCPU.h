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
		void EmulateBIOS();				//инициализация эмулятора значениями

		friend class Interrupts;

	private:

		BYTE MemoryRead(WORD addr);		//чтение байта из памяти
		WORD MemoryReadWord(WORD addr);	//чтение слова из памяти
		void MemoryWrite(WORD addr, BYTE value);	//запись байта в память
		void MemoryWriteWord(WORD addr, WORD value);//запись слова в память

		void INTJump(WORD address);	//переход на стартовый адрес прерывания

		void StackPushByte(BYTE value);	//загрузка байта в стек
		void StackPushWord(WORD value);	//загрузка слова в стек
		BYTE StackPopByte();			//выгрузка байта из стека
		WORD StackPopWord();			//выгрузка слова из стека

		//CPU регистры
		BYTE A;
		BYTE F;//Флаговый регистр
		WordRegister BC;
		WordRegister DE;
		WordRegister HL;
		WORD PC;//Счетчик команд PC. 
		WORD SP;//Указатель стека SP. 
		BYTE IME;//Interrupt master enable register

		bool Halted;	//флаг того, что произошла остановка
		bool HaltBug;	//для предотвращения бага

		BYTE DIDelay;	//задержка при запрете прерываний
		BYTE EIDelay;	//задержка при разрешении прерываний

		Memory &MMU;
		GPU &GPU;
		DividerTimer &DIV;
		Joypad &Joypad;
		Interrupts &INT;
	};

}