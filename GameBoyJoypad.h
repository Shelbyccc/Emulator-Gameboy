#pragma once

#include "GameBoyDefs.h"
#include <memory.h>

namespace GameBoy
{

	class Interrupts;

	class Joypad
	{
	public:
		struct ButtonsState
		{
			BYTE A;
			BYTE B;
			BYTE start;
			BYTE select;
			BYTE left;
			BYTE right;
			BYTE up;
			BYTE down;
		};

		Joypad() { Reset(); }

		void Step(Interrupts &INT);		//действие
		void UpdateJoypad(ButtonsState &buttons) { Buttons = buttons; }	//обновление кнопок

		void Reset() { P1 = 0; memset(&Buttons, 0, sizeof(Buttons)); }	//сброс

		void EmulateBIOS() { Reset(); }		//инициализация

		void P1Changed(BYTE value) { P1 = value; }	//изменение регистра
		BYTE GetP1() { return P1 | 0xC0; }			//значение регистра

	private:

		BYTE P1;//регистр клавиатуры
		//Bit 7 - Not used
		//Bit 6 - Not used
		//Bit 5 - P15 out port
		//Bit 4 - P14 out port
		//Bit 3 - P13 in port
		//Bit 2 - P12 in port
		//Bit 1 - P11 in port
		//Bit 0 - P10 in port
		//
		//		P14			P15
		//		|			|
		//P10-----O-Right-----O-A
		//		|			|
		//P11-----O-Left------O-B
		//		|			|
		//P12-----O-Up--------O-Select
		//		|			|
		//P13-----O-Down------O-Start
		//		|			|

		ButtonsState Buttons;
	};

}