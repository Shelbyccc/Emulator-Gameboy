#include "GameBoyGPU.h"
#include <algorithm>
#include <time.h>
#include <stdio.h>

#include "GameBoyInterrupts.h"
#include "GameBoyMemory.h"

#define LCD_ON() ((LCDC & 0x80) != 0)		//���������, �� �������� �� �������
#define GET_LCD_MODE() ((GameboyLCDModes)(STAT & 0x3))
#define SET_LCD_MODE(value) {STAT &= 0xFC; STAT |= (value);}	//�������� ��������� 
#define GET_TILE_PIXEL(VRAMAddr, x, y) ((((VRAM[(VRAMAddr) + (y) * 2 + 1] >> (7 - (x))) & 0x1) << 1) | ((VRAM[(VRAMAddr) + (y) * 2] >> (7 - (x))) & 0x1))	//�������� ����

GameBoy::GPU::GPU(Interrupts& INT) :
INT(INT)
{
	Reset();
	//�������� �������. � ������ ������ � ��� ����� ������� �������� �������
	GB2RGBPalette[0] = 0xFFE1F7D1;
	GB2RGBPalette[1] = 0xFF87C372;
	GB2RGBPalette[2] = 0xFF337053;
	GB2RGBPalette[3] = 0xFF092021;

	//������������ ��������� � ����������� �� ���-�� ��������
	SpriteClocks[0] = 0;
	SpriteClocks[1] = 8;
	SpriteClocks[2] = 20;
	SpriteClocks[3] = 32;
	SpriteClocks[4] = 44;
	SpriteClocks[5] = 52;
	SpriteClocks[6] = 64;
	SpriteClocks[7] = 76;
	SpriteClocks[8] = 88;
	SpriteClocks[9] = 96;
	SpriteClocks[10] = 108;

	SpriteQueue = std::vector<DWORD>();
	SpriteQueue.reserve(40);
}

//��� ��������� ������ ������ LCD - ���������� �������� ����� ��������� � ����� ������� � 2, 3, 0.
//����� ��������� ��������� ������ �� ��������� � ��������� 1. ����� ��� ���������� ������ � ������ ������.
void GameBoy::GPU::Step(DWORD clockDelta, Memory& MMU)
{
	ClockCounter += clockDelta;

	if (OAMDMAStarted)
	{
		DMAStep(MMU);
	}

	while (ClockCounter >= ClocksToNextState)
	{
		ClockCounter -= ClocksToNextState;

		if (!LCD_ON())
		{
			LY = 0;
			ClocksToNextState = 70224;
			for (int i = 0; i < 144; i++)
			{
				memset(Framebuffer[i], 0xFF, sizeof(Framebuffer[0][0]) * 160);
				memset(Backbuffer[i], 0x0, sizeof(Backbuffer[0][0]) * 160);
			}
			NewFrameReady = true;
			continue;
		}

		//������������������ ��������� ��� �����
		//0	LYXX_OAM->LYXX_OAMRAM->LYXX_HBLANK->LYXX_HBLANK_INC
		//1	LYXX_OAM->LYXX_OAMRAM->LYXX_HBLANK->LYXX_HBLANK_INC
		//...	...
		//143	LYXX_OAM->LYXX_OAMRAM->LYXX_HBLANK->LYXX_HBLANK_INC
		//144	LY9X_VBLANK->LY9X_VBLANK_INC
		//...	...
		//152	LY9X_VBLANK->LY9X_VBLANK_INC
		//153	LY00_VBLANK->LY00_HBLANK

		switch (LCDMode)
		{
		/*��� �������� � ��� ��������� �� �������� � �������� STAT ��������� �� 2 (������ OAM).
		� ���� ��������� �� ��������� ���� ����������� DMG. ���� � �������� SCX ���������� ��� 2 (��������, ������� ����� 4),
		�� ������ ������ ��� ���� ���-�� ��������, ��� ��������� ��������� ������ �������� ���� ������������ �� 4 �����. */
		case LCDMODE_LYXX_OAM:
			SET_LCD_MODE(GBLCDMODE_OAM);
			if ((STAT & 0x20) && !LCDCInterrupted)			//���������, ��������� �� LCDC ����������.
			{
				INT.Request(Interrupts::INTERRUPT_LCDC);	//����������� ���
				LCDCInterrupted = true;						//LCDC ���������� ������ ����������� ������
			}

			//LYC=LY ��������� � ������ ������
			CheckLYC(INT);

			LCDMode = LCDMODE_LYXX_OAMRAM;					//������������� ��������� ���������
			ClocksToNextState = 80;							//������ ��������� ������ ����� 80 ������
			break;
		
		/*�������� � STAT ��������� �� 3. LCDC ���������� ���. 
		����� ��������� �������, ������� ���������� ������� ��������, ������� ����� ����� �������� �� ������ ������. */
		case LCDMODE_LYXX_OAMRAM:
			PrepareSpriteQueue();				//������� ��������
			SET_LCD_MODE(GBLCDMODE_OAMRAM);

			LCDMode = LCDMODE_LYXX_HBLANK;		//������������� ��������� ���������
			ClocksToNextState = 172 + SpriteClocks[SpriteQueue.size()];	//������ (172 + ��������) ������
			break;
	
		/*����� �� ������������ ������� ������, ������� ����� ������ �� �������� LY. �������� � �������� STAT ��������� 0.
		����������� LCDC ����������, ���� ��� ��� �� ���� ���������. 
		����� ����� ������������� ���: 200 ������ � ����� ��-�� SCX � ����� ��-�� ��������.
		����� �������, ����� �� ������������ �� �������� � ������������, ������� ��������� � ��������� LCDMODE_LYXX_OAMRAM.*/
		case LCDMODE_LYXX_HBLANK:
			RenderScanline();					//��������� ������

			SET_LCD_MODE(GBLCDMODE_HBLANK);
			if ((STAT & 0x08) && !LCDCInterrupted)		//H-blank ���������� �������
			{
				INT.Request(Interrupts::INTERRUPT_LCDC);
				LCDCInterrupted = true;
			}

			LCDMode = LCDMODE_LYXX_HBLANK_INC;			//��������� ���������
			ClocksToNextState = 200 - SpriteClocks[SpriteQueue.size()];		//200 ������ � ����� ��-�� ��������.
			break;

		//������� �� ��������� ������
		case LCDMODE_LYXX_HBLANK_INC:
			LY++;		//����������� ������� ����� �� 1

			STAT &= 0xFB;		//�������� LYC ���

			LCDCInterrupted = false;	//�������� ����, ������� �������, ��� ���������� ���� ���������

			if (LY == 144)				//���� ���������� ���� �����
			{
				LCDMode = LCDMODE_LY9X_VBLANK;
			}
			else
			{
				LCDMode = LCDMODE_LYXX_OAM;		//����� ��������� � ������ �� ��������� �������
			}
			ClocksToNextState = 4;		//������ 4 �����
			break;

		/*���� �� ������� � ��� ��������� � LY ����� 144, �� ��� ���� ��� - �� ��������, ��� �� ������� � ��������� V - blank.
		������������� � STAT ��������� 1.
		����������� ���������� V - blank.
		����� �� ���� ����� ��������� LCDC ����������, ���� ��� ���������.
		����� �������, ����� ����� ���� ��������� ��� ����������.
		���� LY �� ����� 144, �� ������ ����� ������ �� ����, �.�.�� � ��� � V-blank.*/
		case LCDMODE_LY9X_VBLANK:
			//V-blank ����������
			if (LY == 144)
			{
				SET_LCD_MODE(GBLCDMODE_VBLANK);
				INT.Request(Interrupts::INTERRUPT_VBLANK);

				if (STAT & 0x10)
				{
					INT.Request(Interrupts::INTERRUPT_LCDC);
				}
			}

			//��������� LYC=LY � ������ ����� ������
			CheckLYC(INT);

			LCDMode = LCDMODE_LY9X_VBLANK_INC;
			ClocksToNextState = 452;				//������ 452 �����
			break;

		/* ����� ��� ���� ���������������� LY.
		����� �� ���� ������, ��� ��� LY ������ 153 �� ��������� � ������ ���������, � �� �������� ������ � LY9X_VBLANK
		������ ������ ��������� 4 �����.*/
		case LCDMODE_LY9X_VBLANK_INC:
			LY++;

			STAT &= 0xFB;	//�������� LYC ���

			if (LY == 153)
			{
				LCDMode = LCDMODE_LY00_VBLANK;		//��������� ��� 153 ������
			}
			else
			{
				LCDMode = LCDMODE_LY9X_VBLANK;
			}

			LCDCInterrupted = false;	//���������� ����, ��� LCDC ���������� ���� ��������� 

			ClocksToNextState = 4;		//������������ 4 �����
			break;
		
		//����� ��� ���� �������� LY.
		case LCDMODE_LY00_VBLANK:
			//Checking LYC=LY in the begginng of scanline
			//Here LY = 153
			CheckLYC(INT);

			LY = 0;

			LCDMode = LCDMODE_LY00_HBLANK;
			ClocksToNextState = 452;
			break;

		/*��� ��������� � ����� ���������.��� ������ ����� 4 ����� � ������������� � �������� STAT ��������� 0.
		����� ���� ��� ���������� ������.*/
		case LCDMODE_LY00_HBLANK:
			SET_LCD_MODE(GBLCDMODE_HBLANK);

			LCDCInterrupted = false;

			NewFrameReady = true;
			WindowLine = 0;

			LCDMode = LCDMODE_LYXX_OAM;
			ClocksToNextState = 4;
			break;
		}
	}
}

void GameBoy::GPU::Reset()
{
	memset(VRAM, 0x0, VRAMBankSize * 2);
	memset(OAM, 0x0, 0xFF);
	for (int i = 0; i < 144; i++)
	{
		memset(Framebuffer[i], 0xFF, sizeof(Framebuffer[0][0]) * 160);
		memset(Backbuffer[i], 0, sizeof(Backbuffer[0][0]) * 160);
	}

	LCDC = 0x0;
	STAT = 0x0;
	SCY = 0x0;
	SCX = 0x0;
	BGP = 0x0;
	OBP0 = 0x0;
	OBP1 = 0x0;
	WY = 0x0;
	WX = 0x0;
	SpriteQueue.clear();
	NewFrameReady = false;
	ClockCounter = 0;
	ClocksToNextState = 0x0;
	LCDCInterrupted = false;
	LY = 0x0;
	LYC = 0x0;
	LCDMode = LCDMODE_LYXX_OAM;
	WindowLine = 0;
	OAMDMAStarted = false;
	OAMDMASource = 0;
	OAMDMAProgress = 0;
}

void GameBoy::GPU::EmulateBIOS()
{
	LCDC = 0x91;
	SCY = 0x0;
	SCX = 0x0;
	LYC = 0x0;
	BGP = 0xFC;
	OBP0 = 0xFF;
	OBP1 = 0xFF;
	WY = 0x0;
	WX = 0x0;
}

void GameBoy::GPU::WriteVRAM(WORD addr, BYTE value)				//������ � �����������
{
	if (GET_LCD_MODE() != GBLCDMODE_OAMRAM || !LCD_ON())
	{
		VRAM[addr] = value;
	}
}

void GameBoy::GPU::WriteOAM(BYTE addr, BYTE value)				//������ � ������ ��������
{
	if (GET_LCD_MODE() == GBLCDMODE_HBLANK || GET_LCD_MODE() == GBLCDMODE_VBLANK || !LCD_ON())
	{
		OAM[addr] = value;
	}
}

BYTE GameBoy::GPU::ReadVRAM(WORD addr)							//������ �� �����������
{
	if (GET_LCD_MODE() != GBLCDMODE_OAMRAM || !LCD_ON())
	{
		return VRAM[addr];
	}
	else
	{
		return 0xFF;
	}
}

BYTE GameBoy::GPU::ReadOAM(BYTE addr)							//������ �� ������ ��������
{
	if (GET_LCD_MODE() == GBLCDMODE_HBLANK || GET_LCD_MODE() == GBLCDMODE_VBLANK || !LCD_ON())
	{
		return OAM[addr];
	}
	else
	{
		return 0xFF;
	}
}

void GameBoy::GPU::DMAStep(Memory& MMU)
{
	int bytesToCopy;						//���������� ���� ��� �����������

	OAMDMAStarted = false;
	bytesToCopy = 0xA0 - OAMDMAProgress;	//����� ���������� ���������� ����� ������

	for (int i = 0; i < bytesToCopy; i++, OAMDMAProgress++)				//��������� ������
	{
		OAM[OAMDMAProgress] = MMU.Read(OAMDMASource + OAMDMAProgress);	//�������� ����������� �� ���������
	}
}

void GameBoy::GPU::DMAChanged(BYTE value)
{
	OAMDMAStarted = true;
	OAMDMASource = value << 8;
	OAMDMAProgress = 0;
}

void GameBoy::GPU::LCDCChanged(BYTE value)
{
	//���� LCD ��������
	if (!(value & 0x80))
	{
		STAT &= ~0x3;
		LY = 0;
	}
	//���� LCD �������
	else if ((value & 0x80) && !LCD_ON())
	{
		SpriteQueue.clear();
		NewFrameReady = false;
		ClockCounter = 0;
		LCDCInterrupted = false;

		LY = 0;
		CheckLYC(INT);

		//�����lCD �������, H - blank ������� ������ OAM 80 ������
		SET_LCD_MODE(GBLCDMODE_HBLANK);
		LCDMode = LCDMODE_LYXX_OAMRAM;
		ClocksToNextState = 80;
	}

	if (!(LCDC & 0x20) && (value & 0x20))
	{
		WindowLine = 144;
	}

	LCDC = value;
}

void GameBoy::GPU::STATChanged(BYTE value)
{
	//Coincidence flag and mode flag are read-only
	STAT = (STAT & 0x7) | (value & 0x78);

	//STAT bug
	if (LCD_ON() &&
		(GET_LCD_MODE() == GBLCDMODE_HBLANK || GET_LCD_MODE() == GBLCDMODE_VBLANK))
	{
		INT.Request(Interrupts::INTERRUPT_LCDC);
	}
}

void GameBoy::GPU::LYCChanged(BYTE value)
{
	if (LYC != value)
	{
		LYC = value;
		if (LCDMode != LCDMODE_LYXX_HBLANK_INC && LCDMode != LCDMODE_LY9X_VBLANK_INC)
		{
			CheckLYC(INT);
		}
	}
	else
	{
		LYC = value;
	}
}

//������� ��������� ������
void GameBoy::GPU::RenderScanline()
{
#pragma region background
	if (LCDC & 0x01)	//���� ���� ���������� �� ����� ����
	{
		WORD tileMapAddr = (LCDC & 0x8) ? 0x1C00 : 0x1800;	//����� �������� �������� ��� �������� ����
		WORD tileDataAddr = (LCDC & 0x10) ? 0x0 : 0x800;	//����� ��������������� ��� �������� ��������� ���� � ����(������ ������)

		WORD tileMapX = SCX >> 3;					//�������� ���������� ������ X(�����)
		WORD tileMapY = ((SCY + LY) >> 3) & 0x1F;	//�������� ���������� Y ������(������). 1F = 00011111b, �������� 3 ��������� ������� ����
		BYTE tileMapTileX = SCX & 0x7;				//�������� ���������� ������� ������ �����, �.�. ������� ��������� �����������
		BYTE tileMapTileY = (SCY + LY) & 0x7;

		int tileIdx;
		//���������� ������
		//� ������ 160 ��������
		for (WORD x = 0; x < 160; x++)
		{
			WORD tileAddr = tileMapAddr + tileMapX + tileMapY * 32;	//�������� ����� ������ ������
			//�������� ���� ��� ����
			if (LCDC & 0x10)
			{
				tileIdx = VRAM[tileAddr];							//����� ������ �1 [0;127] 
			}
			else
			{
				tileIdx = (signed char)VRAM[tileAddr] + 128;		//����� ������ �2 [-127;0] - ������ ����� [0,127]
			}
			
			BYTE palIdx = GET_TILE_PIXEL(tileDataAddr + tileIdx * 16, tileMapTileX, tileMapTileY);	//�������� �������� ��� BGP, � ������� ��������
																									//������ ��� ����� �������

			Backbuffer[LY][x] = (BGP >> (palIdx * 2)) & 0x3;			//������� ������ ������������ �������
			Framebuffer[LY][x] = GB2RGBPalette[Backbuffer[LY][x]];		//���� �������� �� �������

			tileMapX = (tileMapX + ((tileMapTileX + 1) >> 3)) & 0x1F;	//��������� � ���������� �������
			tileMapTileX = (tileMapTileX + 1) & 0x7;
		}
	}
	else
	{
		//������� ������� ������
		memset(Framebuffer[LY], 0xFF, sizeof(Framebuffer[0][0]) * 160);
		memset(Backbuffer[LY], 0, sizeof(Backbuffer[0][0]) * 160);
	}
#pragma endregion
#pragma region sprites
	if (LCDC & 0x02)		//���������� ������ �������� �� �����
	{
		BYTE spriteHeight;							//������ ��������
		if (LCDC & 0x04)							//D2 - ������� ������� ��������: 
		{
			spriteHeight = 16;						//8x16
		}
		else
		{
			spriteHeight = 8;						//8x8
		}

		BYTE dmgPalettes[2] = { OBP0, OBP1 };		//�������

		for (int i = SpriteQueue.size() - 1; i >= 0; i--)		
		{
			BYTE spriteAddr = SpriteQueue[i] & 0xFF;

			BYTE dmgPalette = dmgPalettes[(OAM[spriteAddr + 3] >> 4) & 0x1];
			/*
			������ �������� � ������ �� 4 �����
			����	����������
			0		Y ����������
			1		X ����������
			2		����� ����� (0-255)
			3		��� 7: ���������
					��� 6: ���������� ��������� �� ���������, ���� 1
					��� 5: ���������� ��������� �� �����������, ���� 1
					��� 4: ���� 1, ���������� ������� OBJ1, ����� � OBJ0
			*/
			int spriteX = OAM[spriteAddr + 1] - 8;	//�������� ���������� x �� 1-��� �����
			int spritePixelX = 0;
			int spritePixelY = LY - (OAM[spriteAddr] - 16);		//�������� ���������� Y �� 0-��� �����
			int dx = 1;

			if (OAM[spriteAddr + 3] & 0x20)			//��������� �� ��������� (5-�� ��� 3-�� �����)
			{
				spritePixelX = 7 - spritePixelX;
				dx = -1;
			}
			if (OAM[spriteAddr + 3] & 0x40)			//��������� �� ����������� (6-�� ��� 3-�� �����)
			{
				spritePixelY = spriteHeight - 1 - spritePixelY;
			}

			BYTE colorIdx;
			for (int x = spriteX; x < spriteX + 8 && x < 160; x++, spritePixelX += dx)		//��� ���� ������
			{
				colorIdx = GET_TILE_PIXEL((OAM[spriteAddr + 2]) * 16, spritePixelX, spritePixelY);	//�������� ����

				if (colorIdx == 0)//���� 0 ����������
				{
					continue;
				}

				Backbuffer[LY][x] = (dmgPalette >> (colorIdx * 2)) & 0x3;	//������� ������ ������������ �������
				Framebuffer[LY][x] = GB2RGBPalette[Backbuffer[LY][x]];		//���� �������� �� �������
			}
		}
	}
#pragma endregion
}

void GameBoy::GPU::CheckLYC(Interrupts &INT)
{
	if (LYC == LY)
	{
		STAT |= 0x4;	//������������� 2-�� ��� STAT, ���� LYC == LY
		if ((STAT & 0x40) && !LCDCInterrupted)	//���� ���������� 6-�� ��� STAT � �� ���� ����������, �� ���������� ����������
		{
			INT.Request(Interrupts::INTERRUPT_LCDC);	//������ ����������
			LCDCInterrupted = true;
		}
	}
	else
	{
		STAT &= 0xFB;
	}
}

//������� ������������ ������� �������� ��� ������������ ������
void GameBoy::GPU::PrepareSpriteQueue()
{
	SpriteQueue.clear();
	
	//���������� �� ����� ��������
	if (LCD_ON() && (LCDC & 0x2))
	{
		int spriteHeight = (LCDC & 0x4) ? 16 : 8;

		//�������� ������� �� ��������. ��� �� 4, �.�. ������� �������� � ������ �� 4 �����. ����� OAM ����� ������� 40 ��������
		for (BYTE i = 0; i < 160; i += 4)
		{
			if ((OAM[i] - 16) <= LY && LY < (OAM[i] - 16 + spriteHeight))//������� ���������� Y (0 ����)
			{
				SpriteQueue.push_back(i | i);	//������� ����� ������� � OAM

				//�������� 10 ��������
				if (SpriteQueue.size() == 10)
				{
					break;
				}
			}
		}

		//��������� �� ���������� (���������� X)
		if (SpriteQueue.size())
		{
			std::sort(SpriteQueue.begin(), SpriteQueue.end(), std::less<DWORD>());
		}
	}
}
