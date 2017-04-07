#include "GameBoyCPU.h"
#include <stdio.h>
#include <assert.h>
#include <Windows.h>
	
void GameBoy::CPU::Step()
{
	//эмул€цию команд процессора
	switch (opcode)					//код команды
	{
		//8-bit loads

		#pragma region LD nn, n
		//LD nn,n
		//Put value nn into n.
		//nn = B,C,D,E,H,L,BC,DE,HL,SP
		//n = 8 bit immediate value
		case 0x06:
			break;
		case 0x0E:
			break;
		case 0x16:
			break;
		case 0x1E:
			break;
		case 0x26:
			break;
		case 0x2E:
			break;
		#pragma endregion

		#pragma region LD r1, r2
		//LD r1,r2
		//Put value r2 into r1.
		//r1,r2 = A,B,C,D,E,H,L,(HL)
		case 0x7F:
			break;
		case 0x78:
			break;
		case 0x79:;
			break;
		case 0x7A:
			break;
		case 0x7B:
			break;
		case 0x7C:
			break;
		case 0x7D:
			break;
		case 0x7E:
			break;
		case 0x40:
			break;
		case 0x41:
			break;
		case 0x42:
			break;
		case 0x43:
			break;
		case 0x44:
			break;
		case 0x45:
			break;
		case 0x46:
			break;
		case 0x48:
			break;
		case 0x49:
			break;
		case 0x4A:
			break;
		case 0x4B:
			break;
		case 0x4C:
			break;
		case 0x4D:
			break;
		case 0x4E:
			break;
		case 0x50:
			break;
		case 0x51:
			break;
		case 0x52:
			break;
		case 0x53:
			break;
		case 0x54:
			break;
		case 0x55:
			break;
		case 0x56:
			break;
		case 0x58:
			break;
		case 0x59:
			break;
		case 0x5A:
			break;
		case 0x5B:
			break;
		case 0x5C:
			break;
		case 0x5D:
			break;
		case 0x5E:
			break;
		case 0x60:
			break;
		case 0x61:
			break;
		case 0x62:
			break;
		case 0x63:
			break;
		case 0x64:
			break;
		case 0x65:
			break;
		case 0x66:
			break;
		case 0x68:
			break;
		case 0x69:
			break;
		case 0x6A:
			break;
		case 0x6B:
			break;
		case 0x6C:
			break;
		case 0x6D:
			break;
		case 0x6E:
			break;
		case 0x70:
			break;
		case 0x71:
			break;
		case 0x72:
			break;
		case 0x73:
			break;
		case 0x74:
			break;
		case 0x75:
			break;
		case 0x36:
			break;
		#pragma endregion
	
	}
}