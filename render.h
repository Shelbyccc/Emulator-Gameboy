#pragma once

#include <d3d9.h>
#include <d3dx9.h>

extern IDirect3DDevice9* d3dDevice;

//определяем код FVF(Flexible Vertex Format Constants)
const DWORD D3DFVF_TLVERTEX = D3DFVF_XYZRHW | D3DFVF_TEX1;
//D3DFVF_XYZRHW - Формат вершин включает положение преобразованной вершины.
//D3DFVF_TEX1 - Количество координат текстур для этой вершины. 

//определяем пользовательский тип вершин
struct TLVERTEX
{
	float x, y, z;	//координаты в трехмерном пространстве
	float rhw;		//параметр преобразованной вершины
	float u, v;		//координаты текстур
};

bool InitD3D(HWND hWnd);
bool CloseD3D();
void FillVertexBuffer(int width, int height);
IDirect3DTexture9 *CreateBackbuffer(UINT width, UINT height);
void *TextureLock(IDirect3DTexture9 *texture);
void TextureUnlock(IDirect3DTexture9 *texture);
void Render(IDirect3DTexture9 *texture);