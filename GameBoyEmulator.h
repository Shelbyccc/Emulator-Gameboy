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

		const ROMInfo* LoadROM(const wchar_t *ROMPath);	//���������� ��� ���������

		void Step();
		void UpdateJoypad(Joypad::ButtonsState &state);	//�������� ��������� ���������� ��� �������

		void Reset();

		void UseBIOS(bool BIOS);						//������������� ��������� ����������

		const DWORD** GPUFramebuffer();					//��������� �����
		bool IsNewGPUFrameReady();						//�������� ������� ������ �����				
		void WaitForNewGPUFrame();						//����� ����� ������� ������ �����

	private:
		CPU *Cpu;
		GPU *Gpu;
		Interrupts *INT;
		Memory *MMU;
		DividerTimer *DIV;
		Joypad *Input;
	};

}