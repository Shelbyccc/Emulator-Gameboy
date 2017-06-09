#pragma once

#include <d3d9.h>
#include <d3dx9.h>

extern IDirect3DDevice9* d3dDevice;

//���������� ��� FVF(Flexible Vertex Format Constants)
const DWORD D3DFVF_TLVERTEX = D3DFVF_XYZRHW | D3DFVF_TEX1;
//D3DFVF_XYZRHW - ������ ������ �������� ��������� ��������������� �������.
//D3DFVF_TEX1 - ���������� ��������� ������� ��� ���� �������. 

//���������� ���������������� ��� ������
struct TLVERTEX
{
	float x, y, z;	//���������� � ���������� ������������
	float rhw;		//�������� ��������������� �������
	float u, v;		//���������� �������
};

bool InitD3D(HWND hWnd);
bool CloseD3D();
void FillVertexBuffer(int width, int height);
IDirect3DTexture9 *CreateBackbuffer(UINT width, UINT height);
void *TextureLock(IDirect3DTexture9 *texture);
void TextureUnlock(IDirect3DTexture9 *texture);
void Render(IDirect3DTexture9 *texture);