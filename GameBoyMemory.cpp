#include "GameBoyMemory.h"
#include <stdio.h>

#include "GameBoyMBC.h"
#include "GameBoyDividerTimer.h"
#include "GameBoyInterrupts.h"
#include "GameBoyJoypad.h"
#include "GameBoyGPU.h"
#include "GameBoyMBC_ROMOnly.h"
#include "GameBoyMBC1.h"
#include "GameBoyMBC2.h"
#include "GameBoyMBC3.h"
#include "GameBoyMBC5.h"

/*Адреса 0000h - 00FFh: здесь располагается область встроенного ПЗУ системы с программой начального запуска,
предназначенной для считывания информации из картриджа, сравнения ее с записанным образцом и передачи управления в ПЗУ картриджа
по адресу 0100h. После передачи управления внутреннее ПЗУ отключается, и при обращении по этим адресам становится
доступным ПЗУ картриджа.*/
BYTE GameBoy::Memory::BIOS[0x100] = { 
0x31, 0xFE, 0xFF, 0xAF, 0x21, 0xFF, 0x9F, 0x32, 0xCB, 0x7C, 0x20, 0xFB, 0x21, 0x26, 0xFF, 0x0E,
0x11, 0x3E, 0x80, 0x32, 0xE2, 0x0C, 0x3E, 0xF3, 0xE2, 0x32, 0x3E, 0x77, 0x77, 0x3E, 0xFC, 0xE0,
0x47, 0x11, 0x04, 0x01, 0x21, 0x10, 0x80, 0x1A, 0xCD, 0x95, 0x00, 0xCD, 0x96, 0x00, 0x13, 0x7B,
0xFE, 0x34, 0x20, 0xF3, 0x11, 0xD8, 0x00, 0x06, 0x08, 0x1A, 0x13, 0x22, 0x23, 0x05, 0x20, 0xF9,
0x3E, 0x19, 0xEA, 0x10, 0x99, 0x21, 0x2F, 0x99, 0x0E, 0x0C, 0x3D, 0x28, 0x08, 0x32, 0x0D, 0x20,
0xF9, 0x2E, 0x0F, 0x18, 0xF3, 0x67, 0x3E, 0x64, 0x57, 0xE0, 0x42, 0x3E, 0x91, 0xE0, 0x40, 0x04,
0x1E, 0x02, 0x0E, 0x0C, 0xF0, 0x44, 0xFE, 0x90, 0x20, 0xFA, 0x0D, 0x20, 0xF7, 0x1D, 0x20, 0xF2,
0x0E, 0x13, 0x24, 0x7C, 0x1E, 0x83, 0xFE, 0x62, 0x28, 0x06, 0x1E, 0xC1, 0xFE, 0x64, 0x20, 0x06,
0x7B, 0xE2, 0x0C, 0x3E, 0x87, 0xE2, 0xF0, 0x42, 0x90, 0xE0, 0x42, 0x15, 0x20, 0xD2, 0x05, 0x20,
0x4F, 0x16, 0x20, 0x18, 0xCB, 0x4F, 0x06, 0x04, 0xC5, 0xCB, 0x11, 0x17, 0xC1, 0xCB, 0x11, 0x17,
0x05, 0x20, 0xF5, 0x22, 0x23, 0x22, 0x23, 0xC9, 0xCE, 0xED, 0x66, 0x66, 0xCC, 0x0D, 0x00, 0x0B,
0x03, 0x73, 0x00, 0x83, 0x00, 0x0C, 0x00, 0x0D, 0x00, 0x08, 0x11, 0x1F, 0x88, 0x89, 0x00, 0x0E,
0xDC, 0xCC, 0x6E, 0xE6, 0xDD, 0xDD, 0xD9, 0x99, 0xBB, 0xBB, 0x67, 0x63, 0x6E, 0x0E, 0xEC, 0xCC,
0xDD, 0xDC, 0x99, 0x9F, 0xBB, 0xB9, 0x33, 0x3E, 0x3C, 0x42, 0xB9, 0xA5, 0xB9, 0xA5, 0x42, 0x3C,
0x21, 0x04, 0x01, 0x11, 0xA8, 0x00, 0x1A, 0x13, 0xBE, 0x20, 0xFE, 0x23, 0x7D, 0xFE, 0x34, 0x20,
0xF5, 0x06, 0x19, 0x78, 0x86, 0x23, 0x05, 0x20, 0xFB, 0x86, 0x20, 0xFE, 0x3E, 0x01, 0xE0, 0x50 };

//Логотип Nntendo
BYTE GameBoy::Memory::NintendoGraphic[48] = { 
0xCE, 0xED, 0x66, 0x66, 0xCC, 0x0D, 0x00, 0x0B, 0x03, 0x73, 0x00, 0x83, 0x00, 0x0C, 0x00, 0x0D,
0x00, 0x08, 0x11, 0x1F, 0x88, 0x89, 0x00, 0x0E, 0xDC, 0xCC, 0x6E, 0xE6, 0xDD, 0xDD, 0xD9, 0x99,
0xBB, 0xBB, 0x67, 0x63, 0x6E, 0x0E, 0xEC, 0xCC, 0xDD, 0xDC, 0x99, 0x9F, 0xBB, 0xB9, 0x33, 0x3E };

GameBoy::Memory::Memory(
	GameBoy::GPU &GPU,
	GameBoy::DividerTimer &DIV,
	GameBoy::Joypad &joypad,
	GameBoy::Interrupts &INT) :
	GPU(GPU),
	DIV(DIV),
	Joypad(joypad),
	INT(INT),
	InBIOS(true),
	MBC(NULL),
	RAMBanks(NULL),
	ROM(NULL),
	ROMLoaded(false),
	WRAMOffset(WRAMBankSize)
{
	Reset();
}

GameBoy::Memory::~Memory()
{
	SaveRAM();

	if (ROM != NULL)
	{
		delete[] ROM;
	}
	if (RAMBanks != NULL)
	{
		delete[] RAMBanks;
	}
	if (MBC != NULL)
	{
		delete[] MBC;
	}
}

const GameBoy::ROMInfo* GameBoy::Memory::LoadROM(const wchar_t* ROMPath)
{
	FILE* file = NULL;

	_wfopen_s(&file, ROMPath, L"rb");

	if (file == NULL)
	{
		return NULL;
	}

	SaveRAM();
	Reset(true);

	fseek(file, 0, SEEK_END);
	int filesize = ftell(file);
	fseek(file, 0, SEEK_SET);

	ROM = new BYTE[filesize];
	if (fread(ROM, 1, filesize, file) != filesize)
	{
		delete[] ROM;
		ROM = NULL;
		ROMLoaded = false;

		fclose(file);

		return NULL;
	}

	fclose(file);

	//Данные, записанные по адресам 0104h - 0133h, сравниваются с образцом, хранящимся во встроенном ПЗУ.
	//При обнаружении различий игровая приставка прекращает работу.
	for (int i = 0; i < 48; i++)
	{
		if (NintendoGraphic[i] != ROM[i + 0x104])
		{
			delete[] ROM;
			ROM = NULL;
			ROMLoaded = false;

			return NULL;
		}
	}

	//Checking header checksum. 
	//Real Gameboy won't run if it's invalid. We are checking just to be sure that input file is Gameboy ROM
	BYTE Complement = 0;
	//Байты, расположенные по адресам 0134h - 014Dh, последовательно суммируются, и к полученному результату прибавляется 25.
	for (int i = 0x134; i <= 0x14D; i++)
	{
		Complement = Complement + ROM[i];
	}
	Complement += 25;
	//Приставка прекращает работу, если результат предыдущей операции отличается от 0.
	if (Complement)
	{
		delete[] ROM;
		ROM = NULL;
		ROMLoaded = false;

		return NULL;
	}

	//Управление передается по адресу 0100h, и внутреннее ПЗУ отключается.
	LoadedROMInfo.ReadROMInfo(ROM);
	wcsncpy_s(LoadedROMInfo.ROMFile, ROMPath, 250);
	LoadedROMInfo.ROMFile[250] = '\0';

	//Узнаем размер ROM
	if (LoadedROMInfo.ROMSize == 0)
	{
		LoadedROMInfo.ROMSize = filesize / ROMBankSize;
	}

	int RAMSize = (LoadedROMInfo.RAMSize == 0) ? 1 : LoadedROMInfo.RAMSize;
	RAMBanks = new BYTE[RAMSize * RAMBankSize];

	//Инициализируем нужный MBC(Переключатель банков)
	switch (LoadedROMInfo.MMCType)
	{
	case ROMInfo::MMC_ROMONLY:
		MBC = new MBC_ROMOnly(ROM, LoadedROMInfo.ROMSize, RAMBanks, RAMSize);
		break;

	case ROMInfo::MMC_MBC1:
		MBC = new MBC1(ROM, LoadedROMInfo.ROMSize, RAMBanks, RAMSize);
		break;

	case ROMInfo::MMC_MBC2:
		MBC = new MBC2(ROM, LoadedROMInfo.ROMSize, RAMBanks, RAMSize);
		break;

	case ROMInfo::MMC_MBC3:
		MBC = new MBC3(ROM, LoadedROMInfo.ROMSize, RAMBanks, RAMSize);
		break;

	case ROMInfo::MMC_MBC5:
		MBC = new MBC5(ROM, LoadedROMInfo.ROMSize, RAMBanks, RAMSize);
		break;

	default:
		delete[] ROM;
		ROM = NULL;
		ROMLoaded = false;

		return NULL;
	}

	ROMLoaded = true;

	//Восстановление содержимого ОЗУ
	LoadRAM();

	return &LoadedROMInfo;
}

void GameBoy::Memory::Reset(bool clearROM)
{
	if (clearROM)
	{
		if (ROM != NULL)
		{
			delete[] ROM;
		}
		if (RAMBanks != NULL)
		{
			delete[] RAMBanks;
		}
		if (MBC != NULL)
		{
			delete[] MBC;
		}

		ROM = NULL;
		RAMBanks = NULL;
		MBC = NULL;

		ROMLoaded = false;
	}

	WRAMOffset = WRAMBankSize;
	InBIOS = true;
}

void GameBoy::Memory::EmulateBIOS()
{
	InBIOS = false;
}

void GameBoy::Memory::Write(WORD addr, BYTE value)
{
	switch (addr & 0xF000)
	{
		//ROM bank 0
	case 0x0000:
	case 0x1000:
	case 0x2000:
	case 0x3000:
		//Switchable ROM bank
	case 0x4000:
	case 0x5000:
	case 0x6000:
	case 0x7000:
		//Switchable RAM bank
	case 0xA000:
	case 0xB000:
		if (!InBIOS || addr >= 0x100)
		{
			MBC->Write(addr, value);
		}
		break;

		//Video RAM
	case 0x8000:
	case 0x9000:
		GPU.WriteVRAM(addr - 0x8000, value);
		break;

		//Work RAM bank 0
	case 0xC000:
		WRAM[addr - 0xC000] = value;
		break;

		//Switchable work RAM bank
	case 0xD000:
		WRAM[WRAMOffset + (addr - 0xD000)] = value;
		break;

		//Work RAM bank 0 echo
	case 0xE000:
		WRAM[addr - 0xE000] = value;
		break;

	case 0xF000:
		switch (addr & 0xF00)
		{
			//Switchable Work RAM bank echo
		case 0x000:
		case 0x100:
		case 0x200:
		case 0x300:
		case 0x400:
		case 0x500:
		case 0x600:
		case 0x700:
		case 0x800:
		case 0x900:
		case 0xA00:
		case 0xB00:
		case 0xC00:
		case 0xD00:
			WRAM[WRAMOffset + (addr - 0xF000)] = value;
			break;

			//Sprite attrib memory
		case 0xE00:
			GPU.WriteOAM(addr - 0xFE00, value);
			break;

		case 0xF00:
			switch (addr & 0xFF)
			{
			case IOPORT_P1:
				Joypad.P1Changed(value);
				break;

			case IOPORT_DIV:
				DIV.DIVChanged(value);
				break;

			case IOPORT_IF:
				INT.IFChanged(value);
				break;

			case IOPORT_LCDC:
				GPU.LCDCChanged(value);
				break;

			case IOPORT_STAT:
				GPU.STATChanged(value);
				break;

			case IOPORT_SCY:
				GPU.SCYChanged(value);
				break;

			case IOPORT_SCX:
				GPU.SCXChanged(value);
				break;

			case IOPORT_LY:
				GPU.LYChanged(value);
				break;

			case IOPORT_LYC:
				GPU.LYCChanged(value);
				break;

			case IOPORT_DMA:
				GPU.DMAChanged(value);
				break;

			case IOPORT_BGP:
				GPU.BGPChanged(value);
				break;

			case IOPORT_OBP0:
				GPU.OBP0Changed(value);
				break;

			case IOPORT_OBP1:
				GPU.OBP1Changed(value);
				break;

			case IOPORT_WY:
				GPU.WYChanged(value);
				break;

			case IOPORT_WX:
				GPU.WXChanged(value);
				break;

			case 0x50:
				//Writing to 0xFF50 disables BIOS
				//This is done by last instruction in bootstrap ROM
				InBIOS = false;
				break;

			case IOPORT_SVBK:
				WRAMOffset = (value & 0x7) * WRAMBankSize;
				if (WRAMOffset == 0)
				{
					WRAMOffset = WRAMBankSize;
				}
				break;

			case 0xFF:
				INT.IEChanged(value);
				break;

			default:
				if (addr >= 0xFF80 && addr <= 0xFFFE)//Internal RAM
				{
					HRAM[addr - 0xFF80] = value;
				}
				break;
			}
		}
		break;
	}
}

BYTE GameBoy::Memory::Read(WORD addr)
{
	switch (addr & 0xF000)
	{
		//ROM bank 0
	case 0x0000:
	case 0x1000:
	case 0x2000:
	case 0x3000:
		//Switchable ROM bank
	case 0x4000:
	case 0x5000:
	case 0x6000:
	case 0x7000:
		//Switchable RAM bank
	case 0xA000:
	case 0xB000:
		if (InBIOS && addr < 0x100)
		{
			return BIOS[addr];
		}
		else
		{
			return MBC->Read(addr);
		}

		//Video RAM
	case 0x8000:
	case 0x9000:
		return GPU.ReadVRAM(addr - 0x8000);

		//Work RAM bank 0
	case 0xC000:
		return WRAM[addr - 0xC000];

		//Switchable Work RAM bank
	case 0xD000:
		return WRAM[WRAMOffset + (addr - 0xD000)];

		//Work RAM bank 0 echo
	case 0xE000:
		return WRAM[addr - 0xE000];

	case 0xF000:
		switch (addr & 0xF00)
		{
			//Switchable Work RAM bank echo
		case 0x000:
		case 0x100:
		case 0x200:
		case 0x300:
		case 0x400:
		case 0x500:
		case 0x600:
		case 0x700:
		case 0x800:
		case 0x900:
		case 0xA00:
		case 0xB00:
		case 0xC00:
		case 0xD00:
			return WRAM[WRAMOffset + (addr - 0xF000)];

			//Sprite attrib memory
		case 0xE00:
			return GPU.ReadOAM(addr - 0xFE00);

		case 0xF00:
			switch (addr & 0xFF)
			{
			case IOPORT_P1:
				return Joypad.GetP1();

			case IOPORT_DIV:
				return DIV.GetDIV();

			case IOPORT_IF:
				return INT.GetIF();

			case IOPORT_LCDC:
				return GPU.GetLCDC();

			case IOPORT_STAT:
				return GPU.GetSTAT();

			case IOPORT_SCY:
				return GPU.GetSCY();

			case IOPORT_SCX:
				return GPU.GetSCX();

			case IOPORT_LY:
				return GPU.GetLY();

			case IOPORT_LYC:
				return GPU.GetLYC();

			case IOPORT_BGP:
				return GPU.GetBGP();

			case IOPORT_OBP0:
				return GPU.GetOBP0();

			case IOPORT_OBP1:
				return GPU.GetOBP1();

			case IOPORT_WY:
				return GPU.GetWY();

			case IOPORT_WX:
				return GPU.GetWX();

			case IOPORT_SVBK:
				return 0xF8 | ((WRAMOffset / WRAMBankSize) & 0x7);

			case 0xFF:
				return INT.GetIE();

				//Undocumented registers
			case 0x6C:
				return 0xFF;

			case 0x74:
				return 0xFF;

			case 0x76:
				return 0x00;

			case 0x77:
				return 0x00;

			default:
				if (addr >= 0xFF80 && addr <= 0xFFFE)//Internal RAM
				{
					return HRAM[addr - 0xFF80];
				}
				else
				{
					return 0xFF;
				}
			}

		default:
			return 0xFF;
		}
		break;

	default:
		return 0xFF;
	}
}

bool GameBoy::Memory::SaveRAM()
{
	if (!ROMLoaded || !LoadedROMInfo.RAMSize)
	{
		return true;
	}

	wchar_t savefile[255];
	wcsncpy_s(savefile, LoadedROMInfo.ROMFile, 250);
	savefile[250] = '\0';
	wchar_t* dotpos = wcsrchr(savefile, L'.');	//находим точку
	if (dotpos != NULL)
	{
		*dotpos = '\0';					//ставим на место точки знак конца строки
	}
	wcsncat_s(savefile, L".sav", 4);	//добавляем расширение .sav

	return MBC->SaveRAM(savefile, LoadedROMInfo.RAMSize * RAMBankSize);	//вызываем сохранение RAM 
}

bool GameBoy::Memory::LoadRAM()
{
	if (!ROMLoaded || !LoadedROMInfo.RAMSize)
	{
		return true;
	}

	wchar_t savefile[255];
	wcsncpy_s(savefile, LoadedROMInfo.ROMFile, 250);
	savefile[250] = '\0';
	wchar_t* dotpos = wcsrchr(savefile, L'.');
	if (dotpos != NULL)
	{
		*dotpos = '\0';
	}
	wcsncat_s(savefile, L".sav", 4);

	return MBC->LoadRAM(savefile, LoadedROMInfo.RAMSize * RAMBankSize);
}