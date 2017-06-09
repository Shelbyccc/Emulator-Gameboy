#include "GameBoyROMInfo.h"
#include <memory.h>

GameBoy::ROMInfo::ROMInfo()
{
	for (int i = 0; i < 0x101; i++)
	{
		CartridgeTypes[i] = "Unknown";
	}

	CartridgeTypes[CART_ROM_ONLY] = "ROM ONLY";
	CartridgeTypes[CART_ROM_MBC1] = "ROM+MBC1";
	CartridgeTypes[CART_ROM_MBC1_RAM] = "ROM+MBC1+RAM";
	CartridgeTypes[CART_ROM_MBC1_RAM_BATT] = "ROM+MBC1+RAM+BATT";
	CartridgeTypes[CART_ROM_MBC2] = "ROM+MBC2";
	CartridgeTypes[CART_ROM_MBC2_BATT] = "ROM+MBC2+BATT";
	CartridgeTypes[CART_ROM_RAM] = "ROM+RAM";
	CartridgeTypes[CART_ROM_RAM_BATT] = "ROM+RAM+BATT";
	CartridgeTypes[CART_ROM_MMM01] = "ROM+MMM01";
	CartridgeTypes[CART_ROM_MMM01_SRAM] = "ROM+MMM01+SRAM";
	CartridgeTypes[CART_ROM_MMM01_SRAM_BATT] = "ROM+MMM01+SRAM+BATT";
	CartridgeTypes[CART_ROM_MBC3_TIMER_BATT] = "ROM+MBC3+TIMER+BATT";
	CartridgeTypes[CART_ROM_MBC3_TIMER_RAM_BATT] = "ROM+MBC3+TIMER+RAM+BATT";
	CartridgeTypes[CART_ROM_MBC3] = "ROM+MBC3";
	CartridgeTypes[CART_ROM_MBC3_RAM] = "ROM+MBC3+RAM";
	CartridgeTypes[CART_ROM_MBC3_RAM_BATT] = "ROM+MBC3+RAM+BATT";
	CartridgeTypes[CART_ROM_MBC5] = "ROM+MBC5";
	CartridgeTypes[CART_ROM_MBC5_RAM] = "ROM+MBC5+RAM";
	CartridgeTypes[CART_ROM_MBC5_RAM_BATT] = "ROM+MBC5+RAM+BATT";
	CartridgeTypes[CART_ROM_MBC5_RUMBLE] = "ROM+MBC5+RUMBLE";
	CartridgeTypes[CART_ROM_MBC5_RUMBLE_SRAM] = "ROM+MBC5+RUMBLE+SRAM";
	CartridgeTypes[CART_ROM_MBC5_RUMBLE_SRAM_BATT] = "ROM+MBC5+RUMBLE+SRAM+BATT";
	CartridgeTypes[CART_POCKET_CAMERA] = "Pocket Camera";
	CartridgeTypes[CART_BANDAI_TAMA5] = "Bandai TAMA5";
	CartridgeTypes[CART_HUDSON_HUC3] = "Hudson HuC-3";
	CartridgeTypes[CART_HUDSON_HUC1] = "Hudson HuC-1";

	memset(ROMSizes, 0, sizeof(int)* 0xFF);
	ROMSizes[ROMSIZE_2BANK] = 2;
	ROMSizes[ROMSIZE_4BANK] = 4;
	ROMSizes[ROMSIZE_8BANK] = 8;
	ROMSizes[ROMSIZE_16BANK] = 16;
	ROMSizes[ROMSIZE_32BANK] = 32;
	ROMSizes[ROMSIZE_64BANK] = 64;
	ROMSizes[ROMSIZE_128BANK] = 128;
	ROMSizes[ROMSIZE_72BANK] = 72;
	ROMSizes[ROMSIZE_80BANK] = 80;
	ROMSizes[ROMSIZE_96BANK] = 96;

	memset(RAMSizes, 0, sizeof(int)* 0xFF);
	RAMSizes[RAMSIZE_NONE] = 0;
	RAMSizes[RAMSIZE_HALFBANK] = 1;
	RAMSizes[RAMSIZE_1BANK] = 1;
	RAMSizes[RAMSIZE_4BANK] = 4;
	RAMSizes[RAMSIZE_16BANK] = 16;
}

void GameBoy::ROMInfo::ReadROMInfo(const BYTE* ROMBytes)
{
	memcpy(gameTitle, ROMBytes + 0x134, 16);

	ROMSize = ROMSizes[ROMBytes[0x148]];
	RAMSize = RAMSizes[ROMBytes[0x149]];

	cartType = (CartridgeTypesEnum)ROMBytes[0x147];
	if (cartType > CART_UNKNOWN)
	{
		cartType = CART_UNKNOWN;
	}

	//Установка типа картриджа
	switch (cartType)
	{
	case CART_ROM_ONLY:
		MMCType = MMC_ROMONLY;
		break;

	case CART_ROM_MBC1:
	case CART_ROM_MBC1_RAM:
		MMCType = MMC_MBC1;
		break;
	case CART_ROM_MBC1_RAM_BATT:
		MMCType = MMC_MBC1;
		break;

	case CART_ROM_MBC2:
		MMCType = MMC_MBC2;
		break;
	case CART_ROM_MBC2_BATT:
		MMCType = MMC_MBC2;
		break;

	case CART_ROM_RAM:
		MMCType = MMC_ROMONLY;
		break;
	case CART_ROM_RAM_BATT:
		MMCType = MMC_ROMONLY;
		break;

	case CART_ROM_MMM01:
	case CART_ROM_MMM01_SRAM:
		MMCType = MMC_MMM01;
		break;
	case CART_ROM_MMM01_SRAM_BATT:
		MMCType = MMC_MMM01;
		break;

	case CART_ROM_MBC3:
	case CART_ROM_MBC3_RAM:
		MMCType = MMC_MBC3;
		break;
	case CART_ROM_MBC3_RAM_BATT:
		MMCType = MMC_MBC3;
		break;
	case CART_ROM_MBC3_TIMER_BATT:
	case CART_ROM_MBC3_TIMER_RAM_BATT:
		MMCType = MMC_MBC3;
		break;

	case CART_ROM_MBC5:
	case CART_ROM_MBC5_RAM:
		MMCType = MMC_MBC5;
		break;
	case CART_ROM_MBC5_RAM_BATT:
		MMCType = MMC_MBC5;
		break;
	case CART_ROM_MBC5_RUMBLE:
	case CART_ROM_MBC5_RUMBLE_SRAM:
		MMCType = MMC_MBC5;
		break;
	case CART_ROM_MBC5_RUMBLE_SRAM_BATT:
		MMCType = MMC_MBC5;
		break;

	case CART_HUDSON_HUC3:
	case CART_HUDSON_HUC1:
		MMCType = MMC_MBC1;
		break;

	case CART_POCKET_CAMERA:
	case CART_BANDAI_TAMA5:
		break;

	default:
		MMCType = MMC_UNKNOWN;
	}

	if (MMCType == MMC_MBC2)
	{
		RAMSize = 512;
	}
}