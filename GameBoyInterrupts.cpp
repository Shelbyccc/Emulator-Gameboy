#include "GameBoyInterrupts.h"
#include "GameBoyCPU.h"

/*
Прерывание			Приоритет	Стартовый адрес
V-Blank				1			$0040 (Кадровая синхронизация)
LCDC Status			2			$0048 - режимы 0, 1, 2, LYC=LY совпадение (по выбору) (Видеопроцессор)
Timer Overflow		3			$0050 (Переполнение таймера)
Serial Transfer		4			$0058 - когда передача байта завершена (Последовательный интерфейс)
Hi-Lo of P10-P13	5			$0060 (Клавиатура)
*/
void GameBoy::Interrupts::Step(GameBoy::CPU &CPU)
{
	//Мы не можем проверить IME здесь
	if ((IE & IF) & 0x1F)	//1Fh = 00011111b
	{
		//Прерывание отключает HALT, даже если оно не будет обслуживаться из-за IME
		if (CPU.Halted)
		{
			CPU.Halted = false;
			CPU.PC++;
		}

		//Теперь мы проверяем IME на обслуживание прерываний. Переход к вектору прерывания занимает 20 тактов
		if (CPU.IME)
		{
			CPU.IME = 0;	//Сбрасывается флаг IME, и запрещается прием новых запросов на прерывание. 

			CPU.StackPushWord(CPU.PC);		//Состояние счетчика команд (PC) сохраняется в стеке.

			if ((IF & INTERRUPT_VBLANK) && (IE & INTERRUPT_VBLANK))//V-blank interrupt (Кадровая синхронизация)
			{
				IF = 0;	//убираем запрос на прерывание

				CPU.INTJump(0x40);	//Управление передается на стартовый адрес процедуры обслуживания соответствующего прерывания.
			}
			else if ((IF & INTERRUPT_LCDC) && (IE & INTERRUPT_LCDC))//LCDC status (Видеопроцессор)
			{
				IF = 0;

				CPU.INTJump(0x48);
			}
			else if ((IF & INTERRUPT_TIMA) && (IE & INTERRUPT_TIMA))//Timer overflow (Переполнение таймера)
			{
				IF = 0;

				CPU.INTJump(0x50);
			}
			else if ((IF & INTERRUPT_SERIAL) && (IE & INTERRUPT_SERIAL))//Serial transfer (контроллер последовательного интерфейса)
			{
				IF = 0;

				CPU.INTJump(0x58);
			}
			else if ((IF & INTERRUPT_JOYPAD) && (IE & INTERRUPT_JOYPAD))//Joypad (Клавиатура)
			{
				IF = 0;

				CPU.INTJump(0x60);
			}
		}
	}
}