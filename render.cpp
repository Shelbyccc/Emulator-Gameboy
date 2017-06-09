#include "render.h"

IDirect3DDevice9* d3dDevice;			//Интерфейс устройства рендеринга
IDirect3D9* d3d;						//Интерфейс для создания устройства рендеринга
D3DPRESENT_PARAMETERS d3dPresent;		//Структура с помощью которой передаем информацию устройству рендеринга при его создании
IDirect3DVertexBuffer9* vertexBuffer;

bool InitD3D(HWND hWnd)
{
	HRESULT hr;										//Создаем переменную для записи в неё результатов работы функций

	d3d = Direct3DCreate9(D3D_SDK_VERSION);			//Создаем интерфейс Direct3D

	if (!d3d)
	{
		return false;
	}

	ZeroMemory(&d3dPresent, sizeof(d3dPresent));
	d3dPresent.SwapEffect = D3DSWAPEFFECT_FLIP;	//режим смены кадров
	d3dPresent.hDeviceWindow = hWnd;			//дескриптор окна
	d3dPresent.BackBufferCount = 1;				//кол-во задних бууферов
	d3dPresent.PresentationInterval = D3DPRESENT_INTERVAL_IMMEDIATE;		//интервал между кадрами(немедленное обновление окна)
	d3dPresent.Windowed = true;					//оконный режим
	//от ширины и высоты заднего буфера зависит четкость
	d3dPresent.BackBufferWidth = 160*5;		//ширина заднего буфера
	d3dPresent.BackBufferHeight = 144*5;	//высота заднего буфера

	//Создает устройство рендеринга.
	hr = d3d->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, hWnd, D3DCREATE_HARDWARE_VERTEXPROCESSING, &d3dPresent, &d3dDevice);
	//D3DADAPTER_DEFAULT - используем первичную видеокарту
	//D3DDEVTYPE_HAL - устройство рендеринга использует возможности видеокарты
	//hWnd - дескриптор окна
	//D3DCREATE_HARDWARE_VERTEXPROCESSING - обрабатываем вершины видеокартой
	//&d3dPresent - отдаем параметры устройства
	//&d3dDevice - создаем устройство рендеринга

	if (FAILED(hr))
	{
		return false;
	}

	d3dDevice->SetFVF(D3DFVF_TLVERTEX); //устанавливаем формат вершин

	//Создаем буфер для четырех вершин
	d3dDevice->CreateVertexBuffer(sizeof(TLVERTEX)* 4, NULL, D3DFVF_TLVERTEX, D3DPOOL_MANAGED, &vertexBuffer, NULL);
	d3dDevice->SetStreamSource(0, vertexBuffer, 0, sizeof(TLVERTEX));	//Связываем буфер вершин с потоком данных устройства.
	//0 - создается один поток
	//vertexBuffer - указатель на буфер вершин
	//0 - смещение от начала потока до начала данных вершин
	//sizeof(TLVERTEX) - шаг в байтах от одной вершины до другой

	FillVertexBuffer(d3dPresent.BackBufferWidth, d3dPresent.BackBufferHeight);

	return true;
}

bool CloseD3D()
{
	if (vertexBuffer)
	{
		vertexBuffer->Release();
	}

	if (d3dDevice)
	{
		d3dDevice->Release();
	}

	if (d3d)
	{
		d3d->Release();
	}

	return false;
}

//установка значений для вершин
void FillVertexBuffer(int width, int height)
{
	float x = 0, y = 0;

	TLVERTEX* vertices;

	vertexBuffer->Lock(0, 0, (void**)&vertices, NULL);	//блокируем буфер вершин
	//квадрат состоит их двух трегольников
	//левый верхний угол
	vertices[0].x = x - 0.5f;
	vertices[0].y = y - 0.5f;
	vertices[0].z = 0.0f;
	vertices[0].rhw = 1.0f;
	vertices[0].u = 0.0f;	//растягивание текстуры по x
	vertices[0].v = 0.0f;	//по y
	//правый верхний угол
	vertices[1].x = x + width - 0.5f;
	vertices[1].y = y - 0.5f;
	vertices[1].z = 0.0f;
	vertices[1].rhw = 1.0f;
	vertices[1].u = 1.0f;
	vertices[1].v = 0.0f;
	//правый нижний угол
	vertices[2].x = x + width - 0.5f;
	vertices[2].y = y + height - 0.5f;
	vertices[2].z = 0.0f;
	vertices[2].rhw = 1.0f;
	vertices[2].u = 1.0f;
	vertices[2].v = 1.0f;
	//левый нижний угол
	vertices[3].x = x - 0.5f;
	vertices[3].y = y + height - 0.5f;
	vertices[3].z = 0.0f;
	vertices[3].rhw = 1.0f;
	vertices[3].u = 0.0f;
	vertices[3].v = 1.0f;

	vertexBuffer->Unlock();								//разблокируем буфер вершин
}

IDirect3DTexture9 *CreateBackbuffer(UINT width, UINT height)
{
	IDirect3DTexture9 *backbuffer;

	if (FAILED(D3DXCreateTexture(d3dDevice, 160, 144, 1, 0, D3DFMT_A8R8G8B8, D3DPOOL_MANAGED, &backbuffer)))
	{
		return 0;
	}

	return backbuffer;
}

void *TextureLock(IDirect3DTexture9 *texture)				//блокировка прямоуголного участка (нашего окна)
{
	D3DLOCKED_RECT rect;

	if (FAILED(texture->LockRect(0, &rect, NULL, 0)))
	{
		return 0;
	}

	return rect.pBits;										//указатель на заблокированные биты
}

void TextureUnlock(IDirect3DTexture9 *texture)
{
	texture->UnlockRect(0);
}

void Render(IDirect3DTexture9 *texture)
{
	d3dDevice->Clear(0, NULL, D3DCLEAR_TARGET, 0xFF000000, 0.0f, 0);	//очистка
	d3dDevice->BeginScene();											//начало сцены

	d3dDevice->SetTexture(0, texture);									//накладываем текстуру
	d3dDevice->DrawPrimitive(D3DPT_TRIANGLEFAN, 0, 2);					//выводим примитив
	//D3DPT_TRIANGLEFAN - треугольник
	//0 - индекс первой вершины
	//2 - выводим два треугольника

	d3dDevice->EndScene();												//конец сцены
	d3dDevice->Present(NULL, NULL, NULL, NULL);
}