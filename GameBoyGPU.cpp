#include "GameBoyGPU.h"
#include <algorithm>
#include <time.h>
#include <stdio.h>

#include "GameBoyInterrupts.h"
#include "GameBoyMemory.h"

#define LCD_ON() ((LCDC & 0x80) != 0)		//проверяем, не отключен ли дисплей
#define GET_LCD_MODE() ((GameboyLCDModes)(STAT & 0x3))
#define SET_LCD_MODE(value) {STAT &= 0xFC; STAT |= (value);}	//изменяем состояние 
#define GET_TILE_PIXEL(VRAMAddr, x, y) ((((VRAM[(VRAMAddr) + (y) * 2 + 1] >> (7 - (x))) & 0x1) << 1) | ((VRAM[(VRAMAddr) + (y) * 2] >> (7 - (x))) & 0x1))	//получаем цвет

GameBoy::GPU::GPU(Interrupts& INT) :
INT(INT)
{
	Reset();
	//Цветовая палитра. В данном случае у нас будет зеленая цветовая палитра
	GB2RGBPalette[0] = 0xFFE1F7D1;
	GB2RGBPalette[1] = 0xFF87C372;
	GB2RGBPalette[2] = 0xFF337053;
	GB2RGBPalette[3] = 0xFF092021;

	//длительность состояний в зависимости от кол-ва спрайтов
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

//При отрисовке каждой строки LCD - контроллер проходит через состояния в таком порядке – 2, 3, 0.
//После отрисовки последней строки он переходит в состояние 1. Затем все начинается заново с первой строки.
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

		//Последовательность состояний для строк
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
		/*При переходе в это состояние мы изменяем в регистре STAT состояние на 2 (чтение OAM).
		В этом состоянии мы учитываем одну особенность DMG. Если у регистра SCX установлен бит 2 (например, регистр равен 4),
		то именно сейчас нам надо где-то отметить, что следующие состояния должны изменить свою длительность на 4 такта. */
		case LCDMODE_LYXX_OAM:
			SET_LCD_MODE(GBLCDMODE_OAM);
			if ((STAT & 0x20) && !LCDCInterrupted)			//Проверяем, разрешено ли LCDC прерывание.
			{
				INT.Request(Interrupts::INTERRUPT_LCDC);	//запрашиваем его
				LCDCInterrupted = true;						//LCDC прерывание больше запрашивать нельзя
			}

			//LYC=LY проверяем в начале строки
			CheckLYC(INT);

			LCDMode = LCDMODE_LYXX_OAMRAM;					//устанавливаем следующее состояние
			ClocksToNextState = 80;							//Данное состояние длится ровно 80 тактов
			break;
		
		/*Изменяем в STAT состояние на 3. LCDC прерывания нет. 
		Здесь находится функция, которая составляет очередь спрайтов, которые будут позже выведены на данной строке. */
		case LCDMODE_LYXX_OAMRAM:
			PrepareSpriteQueue();				//очередь спрайтов
			SET_LCD_MODE(GBLCDMODE_OAMRAM);

			LCDMode = LCDMODE_LYXX_HBLANK;		//устанавливаем следующее состояние
			ClocksToNextState = 172 + SpriteClocks[SpriteQueue.size()];	//Длится (172 + спрайтов) циклов
			break;
	
		/*Здесь мы отрисовываем текущую строку, которую можно узнать из регистра LY. Отмечаем в регистре STAT состояние 0.
		Запрашиваем LCDC прерывание, если оно еще не было запрошено. 
		Длина такта высчитывается так: 200 тактов – такты из-за SCX – такты из-за спрайтов.
		Таким образом, здесь мы компенсируем то смещение в длительности, которое произошло в состоянии LCDMODE_LYXX_OAMRAM.*/
		case LCDMODE_LYXX_HBLANK:
			RenderScanline();					//отрисовка строки

			SET_LCD_MODE(GBLCDMODE_HBLANK);
			if ((STAT & 0x08) && !LCDCInterrupted)		//H-blank прерывание выбрано
			{
				INT.Request(Interrupts::INTERRUPT_LCDC);
				LCDCInterrupted = true;
			}

			LCDMode = LCDMODE_LYXX_HBLANK_INC;			//слудующее состояние
			ClocksToNextState = 200 - SpriteClocks[SpriteQueue.size()];		//200 тактов – такты из-за спрайтов.
			break;

		//переход на следующую строку
		case LCDMODE_LYXX_HBLANK_INC:
			LY++;		//увеличиваем счетчик строк на 1

			STAT &= 0xFB;		//обнуляем LYC бит

			LCDCInterrupted = false;	//обнуляем флаг, который говорит, что прерывание было запрошено

			if (LY == 144)				//если отрисовали весь экран
			{
				LCDMode = LCDMODE_LY9X_VBLANK;
			}
			else
			{
				LCDMode = LCDMODE_LYXX_OAM;		//иначе переходим к работе со следующей строкой
			}
			ClocksToNextState = 4;		//длится 4 такта
			break;

		/*Если мы перешли в это состояние и LY равен 144, то нам надо как - то отменить, что мы перешли в состояние V - blank.
		Устанавливаем в STAT состояние 1.
		Запрашиваем прерывание V - blank.
		Здесь же надо опять запросить LCDC прерывание, если оно разрешено.
		Таким образом, здесь может быть запрошено два прерывания.
		Если LY не равен 144, то ничего этого делать не надо, т.к.мы и так в V-blank.*/
		case LCDMODE_LY9X_VBLANK:
			//V-blank прерывание
			if (LY == 144)
			{
				SET_LCD_MODE(GBLCDMODE_VBLANK);
				INT.Request(Interrupts::INTERRUPT_VBLANK);

				if (STAT & 0x10)
				{
					INT.Request(Interrupts::INTERRUPT_LCDC);
				}
			}

			//проверяем LYC=LY в начале новой строки
			CheckLYC(INT);

			LCDMode = LCDMODE_LY9X_VBLANK_INC;
			ClocksToNextState = 452;				//длится 452 такта
			break;

		/* Здесь нам надо инкрементировать LY.
		Здесь же надо учесть, что при LY равном 153 мы переходим в другое состояние, а не начинаем заново с LY9X_VBLANK
		Длится данное состояние 4 такта.*/
		case LCDMODE_LY9X_VBLANK_INC:
			LY++;

			STAT &= 0xFB;	//обнуляем LYC бит

			if (LY == 153)
			{
				LCDMode = LCDMODE_LY00_VBLANK;		//состояние для 153 строки
			}
			else
			{
				LCDMode = LCDMODE_LY9X_VBLANK;
			}

			LCDCInterrupted = false;	//сбрасываем флаг, что LCDC прерывание было запрошено 

			ClocksToNextState = 4;		//длительность 4 такта
			break;
		
		//Здесь нам надо обнулить LY.
		case LCDMODE_LY00_VBLANK:
			//Checking LYC=LY in the begginng of scanline
			//Here LY = 153
			CheckLYC(INT);

			LY = 0;

			LCDMode = LCDMODE_LY00_HBLANK;
			ClocksToNextState = 452;
			break;

		/*Это последнее в кадре состояние.Оно длится всего 4 такта и устанавливает в регистре STAT состояние 0.
		После него все начинается заново.*/
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

void GameBoy::GPU::WriteVRAM(WORD addr, BYTE value)				//запись в видеопамять
{
	if (GET_LCD_MODE() != GBLCDMODE_OAMRAM || !LCD_ON())
	{
		VRAM[addr] = value;
	}
}

void GameBoy::GPU::WriteOAM(BYTE addr, BYTE value)				//запись в память спрайтов
{
	if (GET_LCD_MODE() == GBLCDMODE_HBLANK || GET_LCD_MODE() == GBLCDMODE_VBLANK || !LCD_ON())
	{
		OAM[addr] = value;
	}
}

BYTE GameBoy::GPU::ReadVRAM(WORD addr)							//чтение из видеопамяти
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

BYTE GameBoy::GPU::ReadOAM(BYTE addr)							//чтение из памяти спрайтов
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
	int bytesToCopy;						//количество байт для копирования

	OAMDMAStarted = false;
	bytesToCopy = 0xA0 - OAMDMAProgress;	//будем копирывать оставшееся число байтов

	for (int i = 0; i < bytesToCopy; i++, OAMDMAProgress++)				//формируем строку
	{
		OAM[OAMDMAProgress] = MMU.Read(OAMDMASource + OAMDMAProgress);	//копируем изображение из источника
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
	//Если LCD выключен
	if (!(value & 0x80))
	{
		STAT &= ~0x3;
		LY = 0;
	}
	//Если LCD включен
	else if ((value & 0x80) && !LCD_ON())
	{
		SpriteQueue.clear();
		NewFrameReady = false;
		ClockCounter = 0;
		LCDCInterrupted = false;

		LY = 0;
		CheckLYC(INT);

		//КогдаlCD включен, H - blank активен вместо OAM 80 тактов
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

//функция отрисовки строки
void GameBoy::GPU::RenderScanline()
{
#pragma region background
	if (LCDC & 0x01)	//если есть разрешение на вывод фона
	{
		WORD tileMapAddr = (LCDC & 0x8) ? 0x1C00 : 0x1800;	//выбор экранной страницы для хранения фона
		WORD tileDataAddr = (LCDC & 0x10) ? 0x0 : 0x800;	//выбор знакогенератора для хранения элементов фона и окна(набора тайлов)

		WORD tileMapX = SCX >> 3;					//получаем координату строки X(слева)
		WORD tileMapY = ((SCY + LY) >> 3) & 0x1F;	//получаем координату Y строки(сверху). 1F = 00011111b, обнуляем 3 пришедших старших бита
		BYTE tileMapTileX = SCX & 0x7;				//получаем координату пикселя внутри тайла, т.к. графика выводится попиксельно
		BYTE tileMapTileY = (SCY + LY) & 0x7;

		int tileIdx;
		//заполнение строки
		//в строке 160 пикселей
		for (WORD x = 0; x < 160; x++)
		{
			WORD tileAddr = tileMapAddr + tileMapX + tileMapY * 32;	//получаем адрес нужной строки
			//Получаем тайл для фона
			if (LCDC & 0x10)
			{
				tileIdx = VRAM[tileAddr];							//Набор тайлов №1 [0;127] 
			}
			else
			{
				tileIdx = (signed char)VRAM[tileAddr] + 128;		//Набор тайлов №2 [-127;0] - делаем номер [0,127]
			}
			
			BYTE palIdx = GET_TILE_PIXEL(tileDataAddr + tileIdx * 16, tileMapTileX, tileMapTileY);	//получаем смещение для BGP, в котором хранится
																									//индекс для цвета пикселя

			Backbuffer[LY][x] = (BGP >> (palIdx * 2)) & 0x3;			//заносим индекс определенной палитры
			Framebuffer[LY][x] = GB2RGBPalette[Backbuffer[LY][x]];		//само значение из палитры

			tileMapX = (tileMapX + ((tileMapTileX + 1) >> 3)) & 0x1F;	//переходим к следующему пикселю
			tileMapTileX = (tileMapTileX + 1) & 0x7;
		}
	}
	else
	{
		//очистка текущей строки
		memset(Framebuffer[LY], 0xFF, sizeof(Framebuffer[0][0]) * 160);
		memset(Backbuffer[LY], 0, sizeof(Backbuffer[0][0]) * 160);
	}
#pragma endregion
#pragma region sprites
	if (LCDC & 0x02)		//разрешение вывода спрайтов на экран
	{
		BYTE spriteHeight;							//высота спрайтов
		if (LCDC & 0x04)							//D2 - задание размера спрайтов: 
		{
			spriteHeight = 16;						//8x16
		}
		else
		{
			spriteHeight = 8;						//8x8
		}

		BYTE dmgPalettes[2] = { OBP0, OBP1 };		//палитры

		for (int i = SpriteQueue.size() - 1; i >= 0; i--)		
		{
			BYTE spriteAddr = SpriteQueue[i] & 0xFF;

			BYTE dmgPalette = dmgPalettes[(OAM[spriteAddr + 3] >> 4) & 0x1];
			/*
			Спрайт хранятся в памяти по 4 байта
			Байт	Назначение
			0		Y координата
			1		X координата
			2		Номер тайла (0-255)
			3		Бит 7: приоритет
					Бит 6: зеркальное отражение по вертикали, если 1
					Бит 5: зеркальное отражение по горизонтали, если 1
					Бит 4: если 1, используем палитру OBJ1, иначе – OBJ0
			*/
			int spriteX = OAM[spriteAddr + 1] - 8;	//получаем координату x из 1-ого байта
			int spritePixelX = 0;
			int spritePixelY = LY - (OAM[spriteAddr] - 16);		//Получаем координату Y из 0-ого байта
			int dx = 1;

			if (OAM[spriteAddr + 3] & 0x20)			//отражение по вертикали (5-ый бит 3-го байта)
			{
				spritePixelX = 7 - spritePixelX;
				dx = -1;
			}
			if (OAM[spriteAddr + 3] & 0x40)			//отражение по горизонтали (6-ой бит 3-го байта)
			{
				spritePixelY = spriteHeight - 1 - spritePixelY;
			}

			BYTE colorIdx;
			for (int x = spriteX; x < spriteX + 8 && x < 160; x++, spritePixelX += dx)		//для всей строки
			{
				colorIdx = GET_TILE_PIXEL((OAM[spriteAddr + 2]) * 16, spritePixelX, spritePixelY);	//получаем цвет

				if (colorIdx == 0)//цвет 0 прозрачный
				{
					continue;
				}

				Backbuffer[LY][x] = (dmgPalette >> (colorIdx * 2)) & 0x3;	//заносим индекс определенной палитры
				Framebuffer[LY][x] = GB2RGBPalette[Backbuffer[LY][x]];		//само значение из палитры
			}
		}
	}
#pragma endregion
}

void GameBoy::GPU::CheckLYC(Interrupts &INT)
{
	if (LYC == LY)
	{
		STAT |= 0x4;	//устанавливаем 2-ой бит STAT, если LYC == LY
		if ((STAT & 0x40) && !LCDCInterrupted)	//если установлен 6-ой бит STAT и не было прерываний, то генерируем прерывание
		{
			INT.Request(Interrupts::INTERRUPT_LCDC);	//запрос прерывания
			LCDCInterrupted = true;
		}
	}
	else
	{
		STAT &= 0xFB;
	}
}

//функция формирования очереди спрайтов для последующего вывода
void GameBoy::GPU::PrepareSpriteQueue()
{
	SpriteQueue.clear();
	
	//разрешение на вывод спрайтов
	if (LCD_ON() && (LCDC & 0x2))
	{
		int spriteHeight = (LCDC & 0x4) ? 16 : 8;

		//Собираем очередь из спрайтов. Шаг по 4, т.к. спрайты занимают в памяти по 4 байта. Всего OAM может хранить 40 спрайтов
		for (BYTE i = 0; i < 160; i += 4)
		{
			if ((OAM[i] - 16) <= LY && LY < (OAM[i] - 16 + spriteHeight))//смотрим координату Y (0 байт)
			{
				SpriteQueue.push_back(i | i);	//заносим номер спрайта в OAM

				//Максимум 10 спрайтов
				if (SpriteQueue.size() == 10)
				{
					break;
				}
			}
		}

		//сортируем по приоритету (координате X)
		if (SpriteQueue.size())
		{
			std::sort(SpriteQueue.begin(), SpriteQueue.end(), std::less<DWORD>());
		}
	}
}
