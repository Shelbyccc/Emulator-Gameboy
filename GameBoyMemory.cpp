#include "GameBoyMemory.h"

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
	//Video RAM
	case 0x8000:
	case 0x9000:
	//Internal RAM 1
	case 0xC000:
	case 0xD000:
	//Echo of Internal RAM 1
	case 0xE000:
	case 0xF000:
		switch (addr & 0xF00)
		{
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
		//OAM
		case 0xE00:
		case 0xF00:
		break;
		}
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
	//Video RAM
	case 0x8000:
	case 0x9000:
	//Internal RAM 1
	case 0xC000:
	case 0xD000:
	//Echo of Internal RAM 1
	case 0xE000:
	case 0xF000:
		switch (addr & 0xF00)
		{
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
		//OAM
		case 0xE00:
		case 0xF00:
		break;
		}
	}
}