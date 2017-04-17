#pragma once

#include "GameBoyDefs.h"
#include "GameBoyMBC.h"

namespace GameBoy
{

/*
0000-3FFF - ROM Bank 00 (Чтение)

4000-7FFF - ROM Bank 01-7F (Чтение)

A000-BFFF - RAM Bank 00-03, если имеется (Чтение/Запись)

0000-1FFF - RAM вкл. (Запись)

2000-3FFF - ROM Bank Number (Запись)

4000-5FFF - RAM Bank Number - or - Верхние биты of ROM Bank Number (Запись)

6000-7FFF - ROM/RAM Выбор режима (Запись)
*/
class MBC1 : public MBC
{
public:
	enum MBC1ModesEnum				//ROM/RAM режимы
	{
		MBC1MODE_16_8 = 0,			//16Mbit ROM/8Kbyte RAM
		MBC1MODE_4_32 = 1			//4Mbit ROM/32KByte RAM
	};

	MBC1(BYTE *ROM, DWORD ROMSize, BYTE *RAMBanks, DWORD RAMSize) : MBC(ROM, ROMSize, RAMBanks, RAMSize)
	{
		RAMEnabled = false;			//RAM отключена
		Mode = MBC1MODE_16_8;		//Режим 16/8
		RAMOffset = 0;				//Смещение RAM нулевое
		ROMOffset = ROMBankSize;	//Смещение ROM = Размеру Банка
	}

	virtual void Write(WORD addr, BYTE value)			//Запись
	{
		switch (addr & 0xF000)
		{
		//RAM вкл/выкл
		case 0x0000:
		case 0x1000:
			RAMEnabled = (value & 0x0F) == 0x0A;		//RAM вкл
			break;

		//установка ROM bank по номеру
		case 0x2000:
		case 0x3000:
			//Установка 5 младших бит смещения (0x1F) ROMOffset без затрагивания 2 MSB (0x60)
			ROMOffset = (((ROMOffset / ROMBankSize) & 0x60) | (value & 0x1F));			//получаем номер
			ROMOffset %= ROMSize;

			if (ROMOffset == 0x00) ROMOffset = 0x01;									//при выборе нулевого банка перенаправляем на следующий
			else if (ROMOffset == 0x20) ROMOffset = 0x21;										
			else if (ROMOffset == 0x40) ROMOffset = 0x41;									
			else if (ROMOffset == 0x60) ROMOffset = 0x61;

			ROMOffset *= ROMBankSize;													//получаем смещение
			break;

		//переключение RAM bank/установка 2 старших битов ROM bank адреса
		case 0x4000:
		case 0x5000:
			if (Mode == MBC1MODE_16_8)
			{
				ROMOffset = ((ROMOffset / ROMBankSize) & 0x1F) | ((value & 0x3) << 5);	//записываем 2 старших бита
				ROMOffset %= ROMSize;

				if (ROMOffset == 0x00) ROMOffset = 0x01;								//при выборе нулевого банка перенаправляем на следующий
				else if (ROMOffset == 0x20) ROMOffset = 0x21;
				else if (ROMOffset == 0x40) ROMOffset = 0x41;
				else if (ROMOffset == 0x60) ROMOffset = 0x61;

				ROMOffset *= ROMBankSize;												//получаем смещение
			}
			else																		//иначе
			{
				RAMOffset = value & 0x3;												//зарисываем номер RAM bank			
				RAMOffset %= RAMSize;
				RAMOffset *= RAMBankSize;
			}
			break;

		//MBC1 переключение режима 
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

		//Переключаемый RAM bank(запись в него значения, если он существует)
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

		return 0xFF;																	//содержимое не определено
	}

private:
	bool RAMEnabled;
	MBC1ModesEnum Mode;	//Выбор режима MBC1
};

}