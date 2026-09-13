/*
**	Command & Conquer Red Alert(tm)
**	Copyright 2025 Electronic Arts Inc.
**
**	This program is free software: you can redistribute it and/or modify
**	it under the terms of the GNU General Public License as published by
**	the Free Software Foundation, either version 3 of the License, or
**	(at your option) any later version.
**
**	This program is distributed in the hope that it will be useful,
**	but WITHOUT ANY WARRANTY; without even the implied warranty of
**	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
**	GNU General Public License for more details.
**
**	You should have received a copy of the GNU General Public License
**	along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

// Sub-milestone 1c-continued fix (Agent/plans_done/code_1c_port_log.md, "DirectDraw 8-bit
// palette wall" follow-up): modern WDDM DirectDraw refuses to create an 8-bit palette-indexed
// surface in ANY capacity - not just an exclusive-fullscreen display mode, but CreateSurface
// itself (DDERR_INVALIDPIXELFORMAT, confirmed by actually running this build against real game
// data and diagnosing a resulting crash). VisiblePage (WIN32LIB/DRAWBUFF/GBUFFER.CPP's
// GraphicBufferClass::Init) now falls back to plain system memory instead, and this file presents
// that plain 8-bit buffer to the real window via a small Direct3D 11 swapchain: the raw palette
// index bytes are uploaded as an R8_UINT texture, the current palette as a 256x1 RGBA8 lookup
// texture, and a two-instruction pixel shader does the index-to-RGB expansion on the GPU. A
// fullscreen-triangle draw plus a small texture upload is cheap enough that calling this once per
// small sub-rectangle Blit() (the game does dozens per frame for HUD updates) is a non-issue - no
// dirty-rect tracking or present throttling needed.
//
// Known, deliberate scope limits (documented rather than solved, matching this session's other
// fixes): MainWindow is a fixed-size WS_POPUP sized to the screen once at creation and never
// resized (CODE/WINSTUB.CPP's Create_Main_Window), so there is no swapchain resize handling; and
// there is no DXGI device-loss/alt-tab recovery. Both are fine for verifying the game actually
// renders, not for shipping polish.

#include "misc.h"
#include <gbuffer.h>
#include <d3d11.h>
#include <d3dcompiler.h>
#include <string.h>

static ID3D11Device				* D3DDevice				= NULL;
static ID3D11DeviceContext		* D3DContext			= NULL;
static IDXGISwapChain			* D3DSwapChain			= NULL;
static ID3D11RenderTargetView	* D3DRenderTargetView	= NULL;

static ID3D11Texture2D				* IndexTexture			= NULL;
static ID3D11ShaderResourceView	* IndexTextureView	= NULL;
static ID3D11Texture2D				* PaletteTexture		= NULL;
static ID3D11ShaderResourceView	* PaletteTextureView	= NULL;

static ID3D11VertexShader	* PresentVertexShader	= NULL;
static ID3D11PixelShader	* PresentPixelShader	= NULL;

static int PresentWidth	= 0;
static int PresentHeight	= 0;

// No vertex/index buffer at all - the classic "one oversized triangle covers the whole viewport"
// trick, driven purely by SV_VertexID.
static char const PresentVertexShaderSource[] =
	"float4 main(uint id : SV_VertexID) : SV_Position\r\n"
	"{\r\n"
	"	float2 pos[3] = { float2(-1,-1), float2(-1,3), float2(3,-1) };\r\n"
	"	return float4(pos[id], 0, 1);\r\n"
	"}\r\n";

// Both texture reads are integer Load()s, not Sample()s (R8_UINT can't be linearly sampled
// anyway), so no sampler-state object is needed either. This is the entire palette expansion.
static char const PresentPixelShaderSource[] =
	"Texture2D<uint> IndexTex : register(t0);\r\n"
	"Texture2D<float4> PaletteTex : register(t1);\r\n"
	"float4 main(float4 pos : SV_Position) : SV_Target\r\n"
	"{\r\n"
	"	uint idx = IndexTex.Load(int3((int)pos.x, (int)pos.y, 0));\r\n"
	"	return PaletteTex.Load(int3((int)idx, 0, 0));\r\n"
	"}\r\n";

static void Report_Shader_Error(HWND hwnd, char const * what, ID3DBlob * error_blob)
{
	if (error_blob) {
		MessageBox(hwnd, (char const *)error_blob->GetBufferPointer(), what, MB_ICONEXCLAMATION | MB_OK);
		error_blob->Release();
	} else {
		MessageBox(hwnd, "D3DCompile failed with no error text.", what, MB_ICONEXCLAMATION | MB_OK);
	}
}

BOOL Init_Present_Device(HWND hwnd, int width, int height)
{
	DXGI_SWAP_CHAIN_DESC swap_desc;
	memset(&swap_desc, 0, sizeof(swap_desc));
	swap_desc.BufferDesc.Width				= width;
	swap_desc.BufferDesc.Height			= height;
	swap_desc.BufferDesc.Format			= DXGI_FORMAT_B8G8R8A8_UNORM;
	swap_desc.BufferDesc.RefreshRate.Numerator		= 60;
	swap_desc.BufferDesc.RefreshRate.Denominator	= 1;
	swap_desc.SampleDesc.Count			= 1;
	swap_desc.SampleDesc.Quality			= 0;
	swap_desc.BufferUsage				= DXGI_USAGE_RENDER_TARGET_OUTPUT;
	swap_desc.BufferCount				= 2;
	swap_desc.OutputWindow				= hwnd;
	swap_desc.Windowed					= TRUE;
	swap_desc.SwapEffect				= DXGI_SWAP_EFFECT_DISCARD;

	// Texture2D<uint>/integer Load() in the pixel shader needs feature level 10.0+ - guaranteed on
	// any GPU capable of running current Windows at all, but ask explicitly rather than risk a
	// software/9.x fallback silently succeeding with a broken shader.
	D3D_FEATURE_LEVEL const requested_levels[] = {
		D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_1, D3D_FEATURE_LEVEL_10_0
	};
	D3D_DRIVER_TYPE const driver_types[] = { D3D_DRIVER_TYPE_HARDWARE, D3D_DRIVER_TYPE_WARP };
	D3D_FEATURE_LEVEL obtained_level;
	HRESULT result = E_FAIL;

	for (int i = 0; i < 2; i++) {
		result = D3D11CreateDeviceAndSwapChain(NULL, driver_types[i], NULL, 0,
			requested_levels, ARRAYSIZE(requested_levels), D3D11_SDK_VERSION,
			&swap_desc, &D3DSwapChain, &D3DDevice, &obtained_level, &D3DContext);
		if (result == S_OK) break;
	}
	if (result != S_OK) {
		Process_DD_Result(result, FALSE);
		return FALSE;
	}

	ID3D11Texture2D * back_buffer = NULL;
	D3DSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void **)&back_buffer);
	D3DDevice->CreateRenderTargetView(back_buffer, NULL, &D3DRenderTargetView);
	back_buffer->Release();

	D3D11_TEXTURE2D_DESC index_desc;
	memset(&index_desc, 0, sizeof(index_desc));
	index_desc.Width			= width;
	index_desc.Height			= height;
	index_desc.MipLevels		= 1;
	index_desc.ArraySize		= 1;
	index_desc.Format			= DXGI_FORMAT_R8_UINT;
	index_desc.SampleDesc.Count	= 1;
	index_desc.Usage			= D3D11_USAGE_DYNAMIC;
	index_desc.BindFlags		= D3D11_BIND_SHADER_RESOURCE;
	index_desc.CPUAccessFlags	= D3D11_CPU_ACCESS_WRITE;
	if (D3DDevice->CreateTexture2D(&index_desc, NULL, &IndexTexture) != S_OK) return FALSE;
	if (D3DDevice->CreateShaderResourceView(IndexTexture, NULL, &IndexTextureView) != S_OK) return FALSE;

	D3D11_TEXTURE2D_DESC palette_desc;
	memset(&palette_desc, 0, sizeof(palette_desc));
	palette_desc.Width			= 256;
	palette_desc.Height			= 1;
	palette_desc.MipLevels		= 1;
	palette_desc.ArraySize		= 1;
	palette_desc.Format			= DXGI_FORMAT_R8G8B8A8_UNORM;
	palette_desc.SampleDesc.Count	= 1;
	palette_desc.Usage			= D3D11_USAGE_DYNAMIC;
	palette_desc.BindFlags		= D3D11_BIND_SHADER_RESOURCE;
	palette_desc.CPUAccessFlags	= D3D11_CPU_ACCESS_WRITE;
	if (D3DDevice->CreateTexture2D(&palette_desc, NULL, &PaletteTexture) != S_OK) return FALSE;
	if (D3DDevice->CreateShaderResourceView(PaletteTexture, NULL, &PaletteTextureView) != S_OK) return FALSE;

	ID3DBlob * vs_blob = NULL;
	ID3DBlob * ps_blob = NULL;
	ID3DBlob * error_blob = NULL;

	if (D3DCompile(PresentVertexShaderSource, sizeof(PresentVertexShaderSource) - 1, NULL, NULL, NULL,
			"main", "vs_4_0", 0, 0, &vs_blob, &error_blob) != S_OK) {
		Report_Shader_Error(hwnd, "D3DCompile (present vertex shader)", error_blob);
		return FALSE;
	}
	if (D3DCompile(PresentPixelShaderSource, sizeof(PresentPixelShaderSource) - 1, NULL, NULL, NULL,
			"main", "ps_4_0", 0, 0, &ps_blob, &error_blob) != S_OK) {
		Report_Shader_Error(hwnd, "D3DCompile (present pixel shader)", error_blob);
		vs_blob->Release();
		return FALSE;
	}

	D3DDevice->CreateVertexShader(vs_blob->GetBufferPointer(), vs_blob->GetBufferSize(), NULL, &PresentVertexShader);
	D3DDevice->CreatePixelShader(ps_blob->GetBufferPointer(), ps_blob->GetBufferSize(), NULL, &PresentPixelShader);
	vs_blob->Release();
	ps_blob->Release();

	PresentWidth	= width;
	PresentHeight	= height;

	return TRUE;
}

void Update_Palette_Texture(PALETTEENTRY const * entries)
{
	if (!D3DContext || !PaletteTexture) return;

	D3D11_MAPPED_SUBRESOURCE mapped;
	if (D3DContext->Map(PaletteTexture, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped) != S_OK) return;

	unsigned char * dst = (unsigned char *)mapped.pData;
	for (int i = 0; i < 256; i++) {
		dst[i * 4 + 0] = entries[i].peRed;
		dst[i * 4 + 1] = entries[i].peGreen;
		dst[i * 4 + 2] = entries[i].peBlue;
		dst[i * 4 + 3] = 255;
	}

	D3DContext->Unmap(PaletteTexture, 0);
}

void Present_VisiblePage(void)
{
	if (!D3DContext || !VisibleGraphicBuffer) return;

	// See GETPIX.CPP's comment for the Width+XAdd+Pitch stride convention this codebase uses, and
	// CLEAR.CPP's comment for why Offset (not Get_Buffer()) is the correct pixel-data pointer.
	unsigned char const * src_base = (unsigned char const *)VisibleGraphicBuffer->Get_Offset();
	int width		= VisibleGraphicBuffer->Get_Width();
	int height		= VisibleGraphicBuffer->Get_Height();
	int src_stride	= width + VisibleGraphicBuffer->Get_XAdd() + VisibleGraphicBuffer->Get_Pitch();

	D3D11_MAPPED_SUBRESOURCE mapped;
	if (D3DContext->Map(IndexTexture, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped) != S_OK) return;

	unsigned char const * src_row = src_base;
	unsigned char * dst_row = (unsigned char *)mapped.pData;
	for (int y = 0; y < height; y++) {
		memcpy(dst_row, src_row, width);
		src_row += src_stride;
		dst_row += mapped.RowPitch;
	}

	D3DContext->Unmap(IndexTexture, 0);

	D3D11_VIEWPORT viewport;
	viewport.TopLeftX	= 0;
	viewport.TopLeftY	= 0;
	viewport.Width		= (float)PresentWidth;
	viewport.Height		= (float)PresentHeight;
	viewport.MinDepth	= 0.0f;
	viewport.MaxDepth	= 1.0f;

	ID3D11ShaderResourceView * views[2] = { IndexTextureView, PaletteTextureView };

	D3DContext->OMSetRenderTargets(1, &D3DRenderTargetView, NULL);
	D3DContext->RSSetViewports(1, &viewport);
	D3DContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	D3DContext->VSSetShader(PresentVertexShader, NULL, 0);
	D3DContext->PSSetShader(PresentPixelShader, NULL, 0);
	D3DContext->PSSetShaderResources(0, 2, views);
	D3DContext->Draw(3, 0);

	// No vsync wait (interval 0): this gets called once per small sub-rectangle Blit() into
	// VisiblePage, dozens of times a frame - each call is a trivial texture upload plus a 3-vertex
	// draw, and staying non-blocking is what avoids needing dirty-rect tracking or throttling.
	D3DSwapChain->Present(0, 0);
}
