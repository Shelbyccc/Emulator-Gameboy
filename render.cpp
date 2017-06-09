#include "render.h"

IDirect3DDevice9* d3dDevice;			//��������� ���������� ����������
IDirect3D9* d3d;						//��������� ��� �������� ���������� ����������
D3DPRESENT_PARAMETERS d3dPresent;		//��������� � ������� ������� �������� ���������� ���������� ���������� ��� ��� ��������
IDirect3DVertexBuffer9* vertexBuffer;

bool InitD3D(HWND hWnd)
{
	HRESULT hr;										//������� ���������� ��� ������ � �� ����������� ������ �������

	d3d = Direct3DCreate9(D3D_SDK_VERSION);			//������� ��������� Direct3D

	if (!d3d)
	{
		return false;
	}

	ZeroMemory(&d3dPresent, sizeof(d3dPresent));
	d3dPresent.SwapEffect = D3DSWAPEFFECT_FLIP;	//����� ����� ������
	d3dPresent.hDeviceWindow = hWnd;			//���������� ����
	d3dPresent.BackBufferCount = 1;				//���-�� ������ ��������
	d3dPresent.PresentationInterval = D3DPRESENT_INTERVAL_IMMEDIATE;		//�������� ����� �������(����������� ���������� ����)
	d3dPresent.Windowed = true;					//������� �����
	//�� ������ � ������ ������� ������ ������� ��������
	d3dPresent.BackBufferWidth = 160*5;		//������ ������� ������
	d3dPresent.BackBufferHeight = 144*5;	//������ ������� ������

	//������� ���������� ����������.
	hr = d3d->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, hWnd, D3DCREATE_HARDWARE_VERTEXPROCESSING, &d3dPresent, &d3dDevice);
	//D3DADAPTER_DEFAULT - ���������� ��������� ����������
	//D3DDEVTYPE_HAL - ���������� ���������� ���������� ����������� ����������
	//hWnd - ���������� ����
	//D3DCREATE_HARDWARE_VERTEXPROCESSING - ������������ ������� �����������
	//&d3dPresent - ������ ��������� ����������
	//&d3dDevice - ������� ���������� ����������

	if (FAILED(hr))
	{
		return false;
	}

	d3dDevice->SetFVF(D3DFVF_TLVERTEX); //������������� ������ ������

	//������� ����� ��� ������� ������
	d3dDevice->CreateVertexBuffer(sizeof(TLVERTEX)* 4, NULL, D3DFVF_TLVERTEX, D3DPOOL_MANAGED, &vertexBuffer, NULL);
	d3dDevice->SetStreamSource(0, vertexBuffer, 0, sizeof(TLVERTEX));	//��������� ����� ������ � ������� ������ ����������.
	//0 - ��������� ���� �����
	//vertexBuffer - ��������� �� ����� ������
	//0 - �������� �� ������ ������ �� ������ ������ ������
	//sizeof(TLVERTEX) - ��� � ������ �� ����� ������� �� ������

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

//��������� �������� ��� ������
void FillVertexBuffer(int width, int height)
{
	float x = 0, y = 0;

	TLVERTEX* vertices;

	vertexBuffer->Lock(0, 0, (void**)&vertices, NULL);	//��������� ����� ������
	//������� ������� �� ���� ������������
	//����� ������� ����
	vertices[0].x = x - 0.5f;
	vertices[0].y = y - 0.5f;
	vertices[0].z = 0.0f;
	vertices[0].rhw = 1.0f;
	vertices[0].u = 0.0f;	//������������ �������� �� x
	vertices[0].v = 0.0f;	//�� y
	//������ ������� ����
	vertices[1].x = x + width - 0.5f;
	vertices[1].y = y - 0.5f;
	vertices[1].z = 0.0f;
	vertices[1].rhw = 1.0f;
	vertices[1].u = 1.0f;
	vertices[1].v = 0.0f;
	//������ ������ ����
	vertices[2].x = x + width - 0.5f;
	vertices[2].y = y + height - 0.5f;
	vertices[2].z = 0.0f;
	vertices[2].rhw = 1.0f;
	vertices[2].u = 1.0f;
	vertices[2].v = 1.0f;
	//����� ������ ����
	vertices[3].x = x - 0.5f;
	vertices[3].y = y + height - 0.5f;
	vertices[3].z = 0.0f;
	vertices[3].rhw = 1.0f;
	vertices[3].u = 0.0f;
	vertices[3].v = 1.0f;

	vertexBuffer->Unlock();								//������������ ����� ������
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

void *TextureLock(IDirect3DTexture9 *texture)				//���������� ������������� ������� (������ ����)
{
	D3DLOCKED_RECT rect;

	if (FAILED(texture->LockRect(0, &rect, NULL, 0)))
	{
		return 0;
	}

	return rect.pBits;										//��������� �� ��������������� ����
}

void TextureUnlock(IDirect3DTexture9 *texture)
{
	texture->UnlockRect(0);
}

void Render(IDirect3DTexture9 *texture)
{
	d3dDevice->Clear(0, NULL, D3DCLEAR_TARGET, 0xFF000000, 0.0f, 0);	//�������
	d3dDevice->BeginScene();											//������ �����

	d3dDevice->SetTexture(0, texture);									//����������� ��������
	d3dDevice->DrawPrimitive(D3DPT_TRIANGLEFAN, 0, 2);					//������� ��������
	//D3DPT_TRIANGLEFAN - �����������
	//0 - ������ ������ �������
	//2 - ������� ��� ������������

	d3dDevice->EndScene();												//����� �����
	d3dDevice->Present(NULL, NULL, NULL, NULL);
}