#include "GameBoyDefs.h"
#include <Windows.h>
#include "GameBoyEmulator.h"
#include "GameBoyROMInfo.h"
#include <io.h>
#include <Fcntl.h>
#include "render.h"

bool Running = true;
HWND hWnd;
IDirect3DTexture9 *backbuffer;
GameBoy::Joypad::ButtonsState Joypad;
GameBoy::Emulator *emulator = NULL;

//������� ��������� ���������
LRESULT CALLBACK WindowProcedure(HWND hWnd, UINT uMessage, WPARAM wParam, LPARAM lParam)
{
	BYTE keyState = 0;

	switch (uMessage)
	{
	case WM_CLOSE:
	case WM_QUIT:
		Running = false;
		break;

	case WM_KEYDOWN:
		keyState = 1;
	case WM_KEYUP:
		switch (wParam)
		{
		case 'Z':
			Joypad.A = keyState;
			break;

		case 'X':
			Joypad.B = keyState;
			break;

		case 'A':
			Joypad.select = keyState;
			break;

		case 'S':
			Joypad.start = keyState;
			break;

		case VK_RIGHT:
			Joypad.right = keyState;
			break;

		case VK_LEFT:
			Joypad.left = keyState;
			break;

		case VK_UP:
			Joypad.up = keyState;
			break;

		case VK_DOWN:
			Joypad.down = keyState;
			break;
		}
		break;
	default:
		break;
	}

	return DefWindowProc(hWnd, uMessage, wParam, lParam);
}

bool MakeWindow(HINSTANCE hInstance)
{
	WNDCLASSEX wc;

	wc.cbSize = sizeof(WNDCLASSEX);			//������ ���������
	wc.style = CS_HREDRAW | CS_VREDRAW;		//����� ������ ����
	wc.lpfnWndProc = WindowProcedure;		//������� ��������� ���������
	wc.cbClsExtra = 0;		//���������� ���������� ������ ��� �������� ����������
	wc.cbWndExtra = 0;		//���������� ���������� ������ ��� �������� ����������
	wc.hInstance = hInstance;	//���������� ����������
	wc.hIcon = NULL;			//����������� ������
	wc.hCursor = NULL;			//����������� ������
	wc.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);	//���� ����� ��������� � ������ ����
	wc.lpszMenuName = NULL;		//�� ���������� ����
	wc.lpszClassName = L"GameBoy";		//�������� ������
	wc.hIconSm = NULL;	//����������� ������

	if (!RegisterClassEx(&wc))	//������������ ����� � Windows
	{
		return false;
	}

	//�������� ����
	hWnd = CreateWindowEx(NULL, L"GameBoy", L"GameBoy", WS_OVERLAPPEDWINDOW, 0, 0, 160*5, 144*5, NULL, NULL, hInstance, NULL);

	if (!hWnd)
	{
		return false;
	}

	return true;
}

int WINAPI CALLBACK WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd)
{
	AllocConsole();
	HANDLE lStdHandle = GetStdHandle(STD_OUTPUT_HANDLE);
	int hConHandle = _open_osfhandle((intptr_t)lStdHandle, _O_TEXT);
	*stdout = *_fdopen(hConHandle, "w");
	setvbuf(stdout, NULL, _IONBF, 0);

	if (!MakeWindow(hInstance))
	{
		return 0;
	}

	if(!InitD3D(hWnd)) return 0;

	ShowWindow(hWnd, SW_SHOW);

	backbuffer = CreateBackbuffer(160, 144);

	emulator = new GameBoy::Emulator();

	OPENFILENAME ofn;					//��������� ��� ������ �����
	wchar_t szFile[1024] = { 0 };

	ZeroMemory(&ofn, sizeof(ofn));
	ofn.lStructSize = sizeof(ofn);		//����� ��������� � ������//�������������� ����
	ofn.hwndOwner = NULL;				//�������������� ���� ��� ���������
	ofn.lpstrFile = szFile;				//����� ��� ������������� ���� "��� �����"
	ofn.nMaxFile = sizeof(szFile);
	ofn.lpstrFilter = L"*.*\0\0";		//���� ����� *.*(������)
	ofn.nFilterIndex = 1;				//�������� ������ ������
	ofn.lpstrFileTitle = NULL;			//��� � ���������� ��� ����
	ofn.nMaxFileTitle = 0;
	ofn.lpstrInitialDir = NULL;			//��������� �����
	ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_EXPLORER;	//1,2 ����� ��������� ������� ������ ������������ �����
	//3 - ������������ ����� ����� �������

	if (GetOpenFileName(&ofn) == FALSE)	//��������� ����
	{
		return 0;
	}

	const GameBoy::ROMInfo *ROM = emulator->LoadROM(ofn.lpstrFile);	//��������� ����
	emulator->UseBIOS(false);

	if (ROM == NULL)
	{
		return 0;
	}

	printf("Game title: %s\n", ROM->gameTitle);
	printf("Cartridge type: %s\n", ROM->CartTypeToString(ROM->cartType));
	printf("ROM size: %d banks\n", ROM->ROMSize);
	printf("RAM size: %d banks\n", ROM->RAMSize);

	memset(&Joypad, 0, sizeof(Joypad));
	while (Running)
	{
		MSG msg;
		if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}

		emulator->UpdateJoypad(Joypad);
		do
		{
			emulator->Step();
		} while (!emulator->IsNewGPUFrameReady());	//���� �� ������� ������ �����
		emulator->WaitForNewGPUFrame();	//����� �������� ����, ���������� ����, ��� ����� ���� ����

		void *pixels = TextureLock(backbuffer);

		memcpy(pixels, emulator->GPUFramebuffer(), sizeof(DWORD)* 160 * 144);	//��������� ����

		TextureUnlock(backbuffer);

		Render(backbuffer);
	}

	delete emulator;
	backbuffer->Release();
	CloseD3D();

	return 0;
}
