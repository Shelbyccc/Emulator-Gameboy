#pragma once

#include "GameBoyDefs.h"
#include "GameBoyGPU.h"
#include "GameBoyJoypad.h"

namespace GameBoy
{
	class CPU;
	class Interrupts;
	class Memory;
	class DividerTimer;
	class Joypad;
	struct ROMInfo;

	class Emulator
	{
	public:
		Emulator();
		~Emulator();

		const ROMInfo* LoadROM(const wchar_t *ROMPath);	//считывание ПЗУ картриджа

		void Step();
		void UpdateJoypad(Joypad::ButtonsState &state);	//изменяем состояния управления при нажатии

		void Reset();

		void UseBIOS(bool BIOS);						//инициализация эмулятора значениями

		const DWORD** GPUFramebuffer();					//получение кадра
		bool IsNewGPUFrameReady();						//проверка наличия нового кадра				
		void WaitForNewGPUFrame();						//сброс флага наличия нового кадра

	private:
		CPU *Cpu;
		GPU *Gpu;
		Interrupts *INT;
		Memory *MMU;
		DividerTimer *DIV;
		Joypad *Input;
	};

}