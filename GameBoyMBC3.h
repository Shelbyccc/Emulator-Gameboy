#pragma once

#include "GameBoyDefs.h"
#include "GameBoyMBC.h"
#include <ctime>

namespace GameBoy
{

/*
RTC-часы реального времни

0000-3FFF - ROM Bank 00 (Чтение)

4000-7FFF - ROM Bank 01-7F (Чтение)

A000-BFFF - RAM Bank 00-03, если есть (Чтение/Запись)
A000-BFFF - RTC Register 08-0C (Чтение/Запись)

0000-1FFF - RAM and Timer Enable (Запись)

2000-3FFF - ROM Bank Number (Запись)

4000-5FFF - RAM Bank Number - or - RTC Register Select (Запись)

6000-7FFF - Latch Clock Data(Фиксатор) (Запись)

The Clock Counter Registers
  08h  RTC S   Секунды   0-59 (0-3Bh)
  09h  RTC M   Минуты  0-59 (0-3Bh)
  0Ah  RTC H   Часы     0-23 (0-17h)
  0Bh  RTC DL  Младшие 8 бит счетчик дней (0-FFh)
  0Ch  RTC DH  Upper 1 bit of Day Counter, Carry Bit, Halt Flag
        Bit 0  Самый старший бит Счетчика Дней (Bit 8)
        Bit 6  Остановка (0=Active, 1=Stop Timer)
        Bit 7  Бит Переноса (1=Counter Overflow)
Флаг остановки должен быть установлен перед записью в регистры RTC.

The Day Counter
Всего 9 бит Счетчика Дней позволяют считать дни в диапазоне от 0-511 (0-1FFh). Бит Переноса Счетчика Дней устанавливается, когда это значение переполняется.
В этом случае Бит Переноса остается установленным, пока программа не сбросит его.
*/
class MBC3 : public MBC
{
public:
	enum MBC3ModesEnum
	{
		RAMBankMapping = 0,
		RTCRegisterMapping = 1
	};

	enum RTCRegistersEnum
	{
		RTC_S = 0,
		RTC_M = 1,
		RTC_H = 2,
		RTC_DL = 3,
		RTC_DH = 4
	};

	MBC3(BYTE *ROM, DWORD ROMSize, BYTE *RAMBanks, DWORD RAMSize) : MBC(ROM, ROMSize, RAMBanks, RAMSize)
	{
		Mode = RAMBankMapping;
		ROMOffset = ROMBankSize;
		RAMOffset = 0;
		RAMRTCEnabled = false;
		LastLatchWrite = 0xFF;
		BaseTime = 0;
		HaltTime = 0;
	}

	virtual void Write(WORD addr, BYTE value)
	{
		switch (addr & 0xF000)
		{
		//RAM/RTC registers enable/disable
		case 0x0000:
		case 0x1000:
			RAMRTCEnabled = (value & 0x0F) == 0x0A;
			break;

		//ROM bank switching
		case 0x2000:
		case 0x3000:
			ROMOffset = value & 0x7F;
			ROMOffset %= ROMSize;

			if (ROMOffset == 0)
			{
				ROMOffset = 1;
			}

			ROMOffset *= ROMBankSize;
			break;

		//RAM bank/RTC register switching
		case 0x4000:
		case 0x5000:
			if ((value & 0xF) <= 0x3)
			{
				Mode = RAMBankMapping;

				RAMOffset = value & 0xF;
				RAMOffset %= RAMSize;
				RAMOffset *= RAMBankSize;
			}
			else if ((value & 0xF) >= 0x8 && (value & 0xF) <= 0xC)
			{
				Mode = RTCRegisterMapping;
				SelectedRTCRegister = (value & 0xF) - 0x8;
			}
			break;

		//RTC data latch
		case 0x6000:
		case 0x7000:
			if (Mode == RTCRegisterMapping)
			{
				if (LastLatchWrite == 0 && value == 1)
				{
					LatchRTCData();
				}
				LastLatchWrite = value;
			}
			break;

		//Switchable RAM bank/RTC register
		case 0xA000:
		case 0xB000:
			if (RAMRTCEnabled)
			{
				if (Mode == RAMBankMapping)
				{
					RAMBanks[RAMOffset + (addr - 0xA000)] = value;
				}
				else if (Mode == RTCRegisterMapping)
				{
					switch (SelectedRTCRegister)
					{
					case RTC_S:
						{
							time_t oldBasetime = (RTCRegisters[RTC_DH] & 0x40) ? HaltTime : std::time(NULL);
							BaseTime += (oldBasetime - BaseTime) % 60;
							BaseTime -= value;
						}
						break;

					case RTC_M:
						{
							time_t oldBasetime = (RTCRegisters[RTC_DH] & 0x40) ? HaltTime : std::time(NULL);
							time_t oldMinutes = ((oldBasetime - BaseTime) / 60) % 60;
							BaseTime += oldMinutes * 60;
							BaseTime -= value * 60;
						}
						break;

					case RTC_H:
						{
							time_t oldBasetime = (RTCRegisters[RTC_DH] & 0x40) ? HaltTime : std::time(NULL);
							time_t oldHours = ((oldBasetime - BaseTime) / 3600) % 24;
							BaseTime += oldHours * 3600;
							BaseTime -= value * 3600;
						}
						break;

					case RTC_DL:
						{
							time_t oldBasetime = (RTCRegisters[RTC_DH] & 0x40) ? HaltTime : std::time(NULL);
							time_t oldLowDays = ((oldBasetime - BaseTime) / 86400) % 0xFF;
							BaseTime += oldLowDays * 86400;
							BaseTime -= value * 86400;
						}
						break;

					case RTC_DH:
						{
							time_t oldBasetime = (RTCRegisters[RTC_DH] & 0x40) ? HaltTime : std::time(NULL);
							time_t oldHighDays = ((oldBasetime - BaseTime) / 86400) & 0x100;
							BaseTime += oldHighDays * 86400;
							BaseTime -= ((value & 0x1) << 8) * 86400;

							if ((RTCRegisters[RTC_DH] ^ value) & 0x40)
							{
								if (value & 0x40)
								{
									HaltTime = std::time(NULL);
								}
								else
								{
									BaseTime +=	std::time(NULL) - HaltTime;
								}
							}
						}
						break;
					}

					RTCRegisters[SelectedRTCRegister] = value;
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

		//Switchable RAM bank/RTC register
		case 0xA000:
		case 0xB000:
			if (RAMRTCEnabled)
			{
				if (Mode == RAMBankMapping)
				{
					return RAMBanks[RAMOffset + (addr - 0xA000)];
				}
				else
				{
					return RTCRegisters[SelectedRTCRegister];
				}
			}
		}

		return 0xFF;
	}

	virtual bool SaveRAM(const char *path, DWORD RAMSize)
	{
		FILE *file = NULL;
		fopen_s(&file, path, "wb");
		if (file == NULL)
		{
			return true;
		}

		fwrite(RAMBanks, RAMSize, 1, file);

		fwrite(&BaseTime, sizeof(BaseTime), 1, file);
		fwrite(&HaltTime, sizeof(HaltTime), 1, file);
		fwrite(RTCRegisters, sizeof(BYTE), 5, file);

		fflush(file);
		fclose(file);

		return false;
	}

	virtual bool LoadRAM(const char *path, DWORD RAMSize)
	{
		FILE *file = NULL;
		fopen_s(&file, path, "rb");
		if (file == NULL)
		{
			return true;
		}

		fread(RAMBanks, RAMSize, 1, file);

		fread(&BaseTime, sizeof(BaseTime), 1, file);
		fread(&HaltTime, sizeof(HaltTime), 1, file);
		fread(RTCRegisters, sizeof(BYTE), 5, file);

		fflush(file);
		fclose(file);

		return false;
	}

private:
	void LatchRTCData()
	{
		time_t passedTime = ((RTCRegisters[RTC_DH] & 0x40) ? HaltTime : std::time(NULL)) - BaseTime;

		if (passedTime > 0x1FF * 86400)
		{
			do
			{
				passedTime -= 0x1FF * 86400;
				BaseTime += 0x1FF * 86400;
			}while (passedTime > 0x1FF * 86400);

			RTCRegisters[RTC_DH] |= 0x80;//Day counter overflow
		}

		RTCRegisters[RTC_DL] = (passedTime / 86400) & 0xFF;
		RTCRegisters[RTC_DH] &= 0xFE;
		RTCRegisters[RTC_DH] |= ((passedTime / 86400) & 0x100) >> 8;
		passedTime %= 86400;

		RTCRegisters[RTC_H] = (passedTime / 3600) & 0xFF;
		passedTime %= 3600;

		RTCRegisters[RTC_M] = (passedTime / 60) & 0xFF;
		passedTime %= 60;

		RTCRegisters[RTC_S] = passedTime & 0xFF;
	}

	BYTE RTCRegisters[5];
	BYTE SelectedRTCRegister;

	MBC3ModesEnum Mode;
	bool RAMRTCEnabled;

	std::time_t BaseTime;
	std::time_t HaltTime;
	BYTE LastLatchWrite;
};

}