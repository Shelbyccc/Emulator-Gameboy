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
		Регистры в "I/O ports" части ОЗУ
		*/
		enum IOPortsEnum
		{
			IOPORT_P1 = 0x00,	//регистр для чтения информации о клавиатуре (R/W)
			IOPORT_DIV = 0x04,	//Регистр делителя частоты. Происходит его установка (R/W)
			IOPORT_IF = 0x0F,	//Флаг прерываний (R/W)
			IOPORT_LCDC = 0x40,	//LCD контроллер (R/W)
			IOPORT_STAT = 0x41,	//LCDC статус (R/W)
			IOPORT_SCY = 0x42,	//вертикальная координата Y (R/W)
			IOPORT_SCX = 0x43,	//горизонтальная координата X (R/W)
			IOPORT_LY = 0x44,	//LCDC Y-координта строки. Запись приводит к обнулению счетчика (R)
			IOPORT_LYC = 0x45,	//для сравнение с LY (R/W)
			IOPORT_DMA = 0x46,	//DMA передача и стартовй адрес (W)
			IOPORT_BGP = 0x47,	//информация о палитре (R/W)
			IOPORT_OBP0 = 0x48,	//данные палитры объектов №1 (R/W)
			IOPORT_OBP1 = 0x49,	//данные палитры объектов №2 (R/W)
			IOPORT_WY = 0x4A,	//позиция окна по Y (R/W)
			IOPORT_WX = 0x4B,	//позиция окна по X (R/W)
			IOPORT_VBK = 0x4F,	//текущий банк видеопамяти
			IOPORT_SVBK = 0x70	//WRAM банк
		};

		Memory(GPU &GPU, DividerTimer &DIV, Joypad &joypad, Interrupts &INT);
		~Memory();

		const ROMInfo* LoadROM(const wchar_t* ROMPath);		//Загрузка ПЗУ
		bool IsROMLoaded() { return ROMLoaded; }			
		void Reset(bool clearROM = false);

		void EmulateBIOS();

		void Write(WORD addr, BYTE value);
		BYTE Read(WORD addr);

		bool SaveRAM();										//Сохранение ОЗУ
		bool LoadRAM();										//Загрузка ОЗУ

	private:
		bool InBIOS;
		static BYTE BIOS[0x100];							//базовая система ввода вывода (256 байт)

		/*
		Секция памяти				Начальный адрес	Конечный адрес
		ROM bank 0					0x0000			0x3FFF
		Switchable ROM bank			0x4000			0x7FFF
		Video RAM					0x8000			0x9FFF
		Switchable RAM bank			0xA000			0xBFFF
		Internal RAM 1				0xC000			0xDFFF
		Echo of Internal RAM 1		0xE000			0xFDFF
		OAM							0xFE00			0xFE9F
		Не используется				0xFEA0			0xFEFF
		I/O ports					0xFF00			0xFF4B
		Не используется				0xFF4C			0xFF7F
		Internal RAM 2				0xFF80			0xFFFE
		Interrupt enable register	0xFFFF			0xFFFF
		*/

		BYTE* ROM;								//ROM banks 0, 1			
		BYTE* RAMBanks;							//Switchable RAM bank		
		BYTE WRAM[WRAMBankSize * 2];			//Internal RAM 1			
		BYTE HRAM[0x7F];						//Internal RAM 2		
		//Остальные участки памяти реализуются в других блоках программы.

		DWORD WRAMOffset;	//указывает на текущий адрес банка WRAM

		MBC *MBC;			//Контроллер банков памяти

		//Картридж
		bool ROMLoaded;						//флаг того, что картридж загрузился
		ROMInfo LoadedROMInfo;				//Информация о картридже
		static BYTE NintendoGraphic[48];	//Логотип Nintendo

		//Аппаратные части
		GPU &GPU;			//видеопроцессор
		Joypad &Joypad;		//клавиатура
		DividerTimer &DIV;	//Счетчик DIV
		Interrupts &INT;	//Прерывания
	};

}