#ifndef MYPLUGIN8_H
#define MYPLUGIN8_H

#include "vdjVideo8.h"

#if (defined(VDJ_WIN64))
	#include <d3d11.h>
	#pragma comment(lib, "d3d11.lib")
	#include <atlbase.h> //we use atl for the CComPtr smart pointer, but this is optional
#elif (defined(VDJ_WIN))
	#define DIRECT3D_VERSION 0x9000
	#include <d3dx9.h>
	#pragma comment(lib, "d3dx9.lib")
#elif (defined(VDJ_MAC))
	#include <OpenGL/OpenGL.h> // you have to import OpenGL.framework in the XCode project
	typedef unsigned long D3DCOLOR; // To use the same approach as Microsoft Direct3D
	#define D3DCOLOR_RGBA(r,g,b,a) ((D3DCOLOR)(((((a)&0xFF)<<24)|(((r)&0xFF)<<16)|((g)&0xFF)<<8)|((b)&0xFF))) 
	#define glColorD3D(d3dcolor) glColor4ub((d3dcolor>>16)&255, (d3dcolor>>8)&255, d3dcolor&255, (d3dcolor>>24)&255 )
#endif

	// Some colors
	const D3DCOLOR white = D3DCOLOR_RGBA(255,255,255,255);
	const D3DCOLOR black = D3DCOLOR_RGBA(0,0,0,255);
	const D3DCOLOR red = D3DCOLOR_RGBA(255,0,0,255);
	const D3DCOLOR green = D3DCOLOR_RGBA(0,255,0,255);
	const D3DCOLOR blue = D3DCOLOR_RGBA(0,0,255,255);
	const D3DCOLOR translucide_black = D3DCOLOR_RGBA(0,0,0,120);


class CMyPlugin8 : public IVdjPluginVideoFx8
{
public:
	HRESULT VDJ_API OnLoad();
	HRESULT VDJ_API OnGetPluginInfo(TVdjPluginInfo8 *infos);
	ULONG VDJ_API Release();
	HRESULT VDJ_API OnDeviceInit();
	HRESULT VDJ_API OnDeviceClose();
	HRESULT VDJ_API OnStart();
	HRESULT VDJ_API OnStop();
	HRESULT VDJ_API OnDraw();

private:
	HRESULT OnVideoResize(int VidWidth, int VidHeight);
	int VideoWidth = 0;
	int VideoHeight = 0;

#if (defined(VDJ_WIN64))

	ID3D11Device* D3DDevice = nullptr;

#elif (defined(VDJ_WIN))

	IDirect3DDevice9*  D3DDevice = nullptr;
	IDirect3DTexture9* D3DTexture = nullptr;
	IDirect3DSurface9* D3DSurface = nullptr;

#elif (defined(VDJ_MAC))

	CGLContextObj glContext = 0;
	GLuint GLTexture = 0;

#endif
};

#endif


MyPlugin8.cpp:
#include "MyPlugin8.h"

#if (defined(VDJ_WIN))
	#ifndef SAFE_RELEASE
		#define SAFE_RELEASE(p) { if (p) { (p)->Release(); (p)=NULL; } }
	#endif
#endif
//-----------------------------------------------------------------------------
HRESULT VDJ_API CMyPlugin8::OnLoad()
{ 
	// ADD YOUR CODE HERE WHEN THE PLUGIN IS CALLED

	return S_OK;
}
//-----------------------------------------------------------------------------
HRESULT VDJ_API CMyPlugin8::OnGetPluginInfo(TVdjPluginInfo8 *infos)
{
	infos->PluginName = "MyPlugin8";
	infos->Author = "Atomix Productions";
	infos->Description = "My first VirtualDJ 8 plugin";
	infos->Version = "1.0";
	infos->Flags = 0x00; // VDJFLAG_PROCESSLAST if you want to ensure that all other effects are processed first
	infos->Bitmap = NULL;

	return S_OK;
}
//---------------------------------------------------------------------------
ULONG VDJ_API CMyPlugin8::Release()
{
	// ADD YOUR CODE HERE WHEN THE PLUGIN IS RELEASED

	delete this;
	return 0;
}
//---------------------------------------------------------------------------
HRESULT VDJ_API CMyPlugin8::OnDeviceInit()
{ 
	// ADD YOUR CODE HERE
	HRESULT hr;

#if (defined(VDJ_WIN64))

	hr = GetDevice(VdjVideoEngineDirectX11, (void**) &D3DDevice); //GetDevice doesn't AddRef(), so we don't need to release D3DDevice later
	if (hr!=S_OK || D3DDevice==nullptr)
		return S_FALSE;

#elif (defined(VDJ_WIN))

	hr = GetDevice(VdjVideoEngineDirectX9, (void**) &D3DDevice); //GetDevice doesn't AddRef(), so we don't need to release D3DDevice later
	if (hr!=S_OK || D3DDevice==nullptr)
		return S_FALSE;

#elif (defined(VDJ_MAC))

	glContext = CGLGetCurrentContext();

#endif

	// Size of the Video Window
	VideoWidth = width;
	VideoHeight = height;

	return S_OK;
}
//---------------------------------------------------------------------------
HRESULT VDJ_API CMyPlugin8::OnDeviceClose()
{ 
	// ADD YOUR CODE HERE
#if (defined(VDJ_WIN64))
	D3DDevice = nullptr; //can no longer be used when device closed
#elif (defined(VDJ_WIN))
	D3DDevice = nullptr;
#elif (defined(VDJ_MAC))
	glContext = 0;
#endif

	return S_OK;
}
//---------------------------------------------------------------------------
HRESULT VDJ_API CMyPlugin8::OnStart()
{ 
	// ADD YOUR CODE HERE

	return S_OK;
}
//---------------------------------------------------------------------------
HRESULT VDJ_API CMyPlugin8::OnStop()
{ 
	// ADD YOUR CODE HERE

	return S_OK;
}
//---------------------------------------------------------------------------
HRESULT VDJ_API CMyPlugin8::OnDraw()
{ 
	// ADD YOUR CODE HERE
	HRESULT hr;
	TVertex *vertices = nullptr;

	if (VideoWidth!=width || VideoHeight!=height)
	{
		hr = OnVideoResize(width,height);
	}

#if (defined(VDJ_WIN64))
	ID3D11ShaderResourceView* textureView = nullptr; //GetTexture doesn't AddRef, so doesn't need to be released
	hr = GetTexture(VdjVideoEngineDirectX11, (void **) & textureView, &vertices);

	CComPtr<ID3D11DeviceContext> devContext; //use smart pointer to automatically release pointer and prevent memory leak
	D3DDevice->GetImmediateContext(&devContext.p);

	//some other interfaces that are often useful to get when dealing with DX11
	CComPtr<ID3D11Resource> textureResource;
	textureView->GetResource(&textureResource);
	if (!textureResource)
		return E_FAIL;
	CComPtr<ID3D11Texture2D> texture;
	textureResource->QueryInterface<ID3D11Texture2D>(&texture);
	if (!texture)
		return E_FAIL;
	D3D11_TEXTURE2D_DESC textureDesc;
	texture->GetDesc(&textureDesc);

#elif (defined(VDJ_WIN))

	hr = GetTexture(VdjVideoEngineDirectX9, (void **) &D3DTexture, &vertices);
	hr = D3DTexture->GetSurfaceLevel(0, &D3DSurface);

	SAFE_RELEASE(D3DTexture);
	SAFE_RELEASE(D3DSurface);

#elif (defined(VDJ_MAC))

	hr = GetTexture(VdjVideoEngineOpenGL, (void **) &GLTexture, &vertices);

	// glEnable(GL_TEXTURE_RECTANGLE_EXT);
	// glBindTexture(GL_TEXTURE_RECTANGLE_EXT, GLTexture);
	// glBegin(GL_TRIANGLE_STRIP);

	//for(int j=0;j<4;j++)
	//{
	//glColorD3D(vertices[j].color);
	//glTexCoord2f(vertices[j].tu, vertices[j].tv);
	//glVertex3f(vertices[j].position.x, vertices[j].position.y, vertices[j].position.z);
	//}

	//glEnd();
#endif


	hr = DrawDeck();
	return S_OK;
}
//---------------------------------------------------------------------------
HRESULT CMyPlugin8::OnVideoResize(int VidWidth, int VidHeight)
{
	// OnDeviceClose();
	// OnDeviceInit();

	VideoWidth = width;
	VideoHeight = height;

	return S_OK;
}
