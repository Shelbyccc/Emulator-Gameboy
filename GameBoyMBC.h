#pragma once

#include "GameBoyDefs.h"
#include <stdio.h>

namespace GameBoy
{

	const int ROMBankSize = 0x4000;	
	const int RAMBankSize = 0x2000;

	class MBC
	{
	public:
		virtual void Write(WORD addr, BYTE value) = 0;	//смена банков памяти
		virtual BYTE Read(WORD addr) = 0;	//чтение из текущего банка памяти

		virtual bool SaveRAM(const wchar_t *path, DWORD RAMSize)	//сохранение ОЗУ
		{
			FILE *file = NULL;
			_wfopen_s(&file, path, L"wb");
			if (file == NULL)
			{
				return true;
			}

			fwrite(RAMBanks, RAMSize, 1, file);

			fflush(file);
			fclose(file);

			return false;
		}

		virtual bool LoadRAM(const wchar_t *path, DWORD RAMSize)	//загрузка ОЗУ
		{
			FILE *file = NULL;
			_wfopen_s(&file, path, L"rb");
			if (file == NULL)
			{
				return true;
			}

			fread(RAMBanks, RAMSize, 1, file);

			fflush(file);
			fclose(file);

			return false;
		}

	protected:
		MBC(BYTE *ROM, DWORD ROMSize, BYTE *RAMBanks, DWORD RAMSize) : ROM(ROM), ROMSize(ROMSize), RAMBanks(RAMBanks), RAMSize(RAMSize) {}

		BYTE *ROM;		//ROM. Здесь у нас находится весь образ игры.
		BYTE *RAMBanks;	//RAMBanks. Здесь находится оперативная память картриджа.

		DWORD ROMOffset;//Смещения, указывающие на текущий банк памяти.
		DWORD RAMOffset;

		DWORD ROMSize;	//Размеры памяти.
		DWORD RAMSize;
	};

}