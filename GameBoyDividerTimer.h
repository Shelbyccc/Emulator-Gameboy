#pragma once

#include "GameBoyDefs.h"

namespace GameBoy
{

	/*
	Таймер работает на частоте 16384 Hz. Не создает прерываний. Счет через каждые 256 тактов
	*/
	class DividerTimer
	{
	public:
		DividerTimer()
		{
			Reset();
		}

		void Step(DWORD clockDelta)
		{
			ClockCounter += clockDelta;

			//DIV увеличивается каждые 256 тактов
			if (ClockCounter > 255)
			{
				int passedPeriods = ClockCounter / 256;
				ClockCounter %= 256;
				DIV += passedPeriods;
			}
		}

		void Reset()
		{
			ClockCounter = 0;
			DIV = 0;
		}

		void EmulateBIOS()
		{
			Reset();
		}

		void DIVChanged(BYTE value)
		{
			DIV = 0;
		}

		BYTE GetDIV()
		{
			return DIV;
		}

	private:
		DWORD ClockCounter;	//счетчик тактов

		BYTE DIV;	
		/*
		Регистр делителя частоты DIV 
		Адрес: FF04h. 
		Тип: запись/чтение. 
		Назначение разрядов. 
		Содержимое регистра увеличивается на 1 с частотой 16384 Гц.
		Запись любого значения по этому адресу приведет к установке в регистр значения 00 
		*/
	};

}