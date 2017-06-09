#include "GameBoyEmulator.h"
#include "GameBoyCPU.h"
#include "GameBoyInterrupts.h"
#include "GameBoyMemory.h"
#include "GameBoyDividerTimer.h"
#include "GameBoyROMInfo.h"

GameBoy::Emulator::Emulator()
{
	INT = new Interrupts();
	Gpu = new GPU(*INT);
	DIV = new DividerTimer();
	Input = new Joypad();
	MMU = new Memory(*Gpu, *DIV, *Input, *INT);
	Cpu = new CPU(*MMU, *Gpu, *DIV, *Input, *INT);
}

GameBoy::Emulator::~Emulator()
{
	delete Cpu;
	delete Gpu;
	delete DIV;
	delete MMU;
	delete INT;
	delete Input;
}

const GameBoy::ROMInfo* GameBoy::Emulator::LoadROM(const wchar_t *ROMPath)
{
	const ROMInfo *info = MMU->LoadROM(ROMPath);

	return info;
}

void GameBoy::Emulator::Step()
{
	Cpu->Step();
}

void GameBoy::Emulator::UpdateJoypad(Joypad::ButtonsState &state)
{
	Input->UpdateJoypad(state);
}

void GameBoy::Emulator::Reset()
{
	Cpu->Reset();
	Gpu->Reset();
	DIV->Reset();
	Input->Reset();
	MMU->Reset();
	INT->Reset();
}

void GameBoy::Emulator::UseBIOS(bool BIOS)
{
	if (BIOS)
	{
		Reset();
	}
	else
	{
		Cpu->EmulateBIOS();
		Gpu->EmulateBIOS();
		DIV->EmulateBIOS();
		Input->EmulateBIOS();
		MMU->EmulateBIOS();
		INT->EmulateBIOS();
	}
}

const DWORD** GameBoy::Emulator::GPUFramebuffer()	//получение кадра из видеопроцессора
{
	return Gpu->GetFramebuffer();
}

bool GameBoy::Emulator::IsNewGPUFrameReady()	//проверка, можно ли получить новый кадр
{
	return Gpu->IsNewFrameReady();
}

void GameBoy::Emulator::WaitForNewGPUFrame()	//установка того, что нового кадра нет
{
	Gpu->WaitForNewFrame();
}
