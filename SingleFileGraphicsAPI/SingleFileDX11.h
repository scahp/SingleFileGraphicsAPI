#pragma once

#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "d3dcompiler.lib")

#include <d3d11.h>
#include <d3dcompiler.h>
#include <directxmath.h>
using namespace DirectX;

#define DX_RELEASE(x) { if (x) { x->Release(); x = nullptr; } }
#define DX_DELETE(x) { if (x) { delete x; x = nullptr; } }
#define DX_DELETE_ARRAY(x) { if (x) { delete[] x; x = nullptr; } }

const char VertexShaderString[] =
"                                                                               \n\
cbuffer MatrixBuffer                                                            \n\
{                                                                               \n\
    matrix WorldMatrix;                                                         \n\
    matrix ViewMatrix;                                                          \n\
    matrix ProjectionMatrix;                                                    \n\
};                                                                              \n\
                                                                                \n\
struct VertexInputType                                                          \n\
{                                                                               \n\
    float3 Position : POSITION;                                                 \n\
    float4 Color : COLOR;                                                       \n\
};                                                                              \n\
                                                                                \n\
struct PixelInputType                                                           \n\
{                                                                               \n\
    float4 Position : SV_POSITION;                                              \n\
    float4 Color : COLOR;                                                       \n\
};                                                                              \n\
                                                                                \n\
PixelInputType ColorVertexShader(VertexInputType Input)                         \n\
{                                                                               \n\
    PixelInputType Output;                                                      \n\
                                                                                \n\
    float4 Pos = float4(Input.Position, 1.0f);                                  \n\
                                                                                \n\
    // 월드, 뷰 그리고 투영 행렬에 대한 정점의 위치를 계산합니다.                       \n\
    Output.Position = mul(Pos, WorldMatrix);                                    \n\
    Output.Position = mul(Output.Position, ViewMatrix);                         \n\
    Output.Position = mul(Output.Position, ProjectionMatrix);                   \n\
                                                                                \n\
    // 픽셀 쉐이더가 사용할 입력 색상                                               \n\
    Output.Color = Input.Color;                                                 \n\
                                                                                \n\
    return Output;                                                              \n\
}                                                                               \n\
";

const char PixelShaderString[] =
" \
struct PixelInputType                                                           \n\
{                                                                               \n\
    float4 Position : SV_POSITION;                                              \n\
    float4 Color : COLOR;                                                       \n\
};                                                                              \n\
                                                                                \n\
float4 ColorPixelShader(PixelInputType Input) : SV_TARGET                       \n\
{                                                                               \n\
    return Input.Color;                                                         \n\
}                                                                               \n\
";


LRESULT CALLBACK SystemClassWndProc(HWND hwnd, UINT umessage, WPARAM wparam, LPARAM lparam);

void Run_Triangle_DX11()
{
	//////////////////////////////////////////////////////////////////////////
	// 1. 기본 변수들 선언
	//////////////////////////////////////////////////////////////////////////
	IDXGISwapChain* SwapChain = nullptr;
	ID3D11Device* Device = nullptr;
	ID3D11DeviceContext* DeviceContext = nullptr;
	ID3D11RenderTargetView* RenderTargetView = nullptr;
	ID3D11Texture2D* DepthStencilBuffer = nullptr;
	ID3D11DepthStencilState* DepthStencilState = nullptr;
	ID3D11DepthStencilView* DepthStencilView = nullptr;
	ID3D11RasterizerState* RasterState = nullptr;
	ID3D11VertexShader* VertexShader = nullptr;
	ID3D11PixelShader* PixelShader = nullptr;
	ID3D11InputLayout* Layout = nullptr;
	ID3D11Buffer* MatrixBuffer = nullptr;
	ID3D11Buffer* VertexBuffer = nullptr;
	ID3D11Buffer* IndexBuffer = nullptr;

	wchar_t ApplicationName[128] = { 0, };
	memset(ApplicationName, 0, _countof(ApplicationName));
	MultiByteToWideChar(CP_ACP, 0, __FUNCTION__, -1, ApplicationName, _countof(ApplicationName) - 1);
	HINSTANCE Instance = 0;
	HWND Hwnd = 0;
	bool IsFullScreen = false;
	bool IsVsync = true;
	char VideoCardDescription[128] = { 0, };
	int VideoCardMemory = 0;

	int PosX = 0;
	int PosY = 0;

	//////////////////////////////////////////////////////////////////////////
	// 2. 윈도우 생성
	//////////////////////////////////////////////////////////////////////////
	// windows 클래스를 아래와 같이 설정합니다.
	WNDCLASSEX wc;
	wc.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
	wc.lpfnWndProc = SystemClassWndProc;
	wc.cbClsExtra = 0;
	wc.cbWndExtra = 0;
	wc.hInstance = Instance;
	wc.hIcon = LoadIcon(NULL, IDI_WINLOGO);
	wc.hIconSm = wc.hIcon;
	wc.hCursor = LoadCursor(NULL, IDC_ARROW);
	wc.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
	wc.lpszMenuName = NULL;
	wc.lpszClassName = ApplicationName;
	wc.cbSize = sizeof(WNDCLASSEX);

	// windows class를 등록합니다
	RegisterClassEx(&wc);

	int ScreenWidth = GetSystemMetrics(SM_CXSCREEN);
	int ScreenHeight = GetSystemMetrics(SM_CYSCREEN);

	if (IsFullScreen)
	{
		DEVMODE ScreenSetting;
		memset(&ScreenSetting, 0, sizeof(ScreenSetting));
		ScreenSetting.dmSize = sizeof(ScreenSetting);
		ScreenSetting.dmPelsWidth = (unsigned long)ScreenWidth;
		ScreenSetting.dmPelsHeight = (unsigned long)ScreenHeight;
		ScreenSetting.dmBitsPerPel = 32;
		ScreenSetting.dmFields = DM_BITSPERPEL | DM_PELSWIDTH | DM_PELSHEIGHT;

		// 풀스크린으로 디스플레이를 설정합니다.
		ChangeDisplaySettings(&ScreenSetting, CDS_FULLSCREEN);
	}
	else
	{
		ScreenWidth = 800;
		ScreenHeight = 600;

		// 윈도우 창을 가로, 세로의 정 가운데 오도록 함
		PosX = (GetSystemMetrics(SM_CXSCREEN) - ScreenWidth) / 2;
		PosY = (GetSystemMetrics(SM_CYSCREEN) - ScreenHeight) / 2;
	}

	Hwnd = CreateWindowEx(WS_EX_APPWINDOW, ApplicationName, ApplicationName, WS_OVERLAPPEDWINDOW,
		PosX, PosY, ScreenWidth, ScreenHeight, NULL, NULL, Instance, NULL);

	// 윈도우를 화면에 표시하고 포커스를 지정합니다
	ShowWindow(Hwnd, SW_SHOW);
	SetForegroundWindow(Hwnd);
	SetFocus(Hwnd);

	//////////////////////////////////////////////////////////////////////////
	// 3. DirectX 11 초기화
	// 	  - 비디오 카드 선택
	//	  - Swapchain 생성
	// 	  - DeviceContext 생성
    // 	  - RenderTarget 생성
	// 	  - DepthBuffer 생성
    // 	  - DepthStencilState 생성
    // 	  - 레스터라이즈 스테이트 생성
    // 	  - 뷰포트 데이터 준비
	//////////////////////////////////////////////////////////////////////////

	IDXGIFactory* Factory = nullptr;
	if (FAILED(CreateDXGIFactory(__uuidof(IDXGIFactory), (void**)&Factory)))		// Create DX Graphic Interface Factory
		return;

	IDXGIAdapter* Adapter = nullptr;
	if (FAILED(Factory->EnumAdapters(0, &Adapter)))									// Create GraphicCard Interface Adapter by using Factory
		return;

	IDXGIOutput* AdapterOutput = nullptr;
	if (FAILED(Adapter->EnumOutputs(0, &AdapterOutput)))							// 출력(모니터)에 대한 첫번째 어뎁터를 지정합니다.
		return;

	unsigned int NumModes = 0;
    if (FAILED(AdapterOutput->GetDisplayModeList(DXGI_FORMAT_R8G8B8A8_UNORM, DXGI_ENUM_MODES_INTERLACED, &NumModes, NULL)))	// 출력(모니터)에 대한 DXGI_FORMAT_R8G8B8A8_UNORM 표시 형식에 맞는 모드 수를 가져옴
		return;

	DXGI_MODE_DESC* DisplayModeList = new DXGI_MODE_DESC[NumModes];					// 가능한 모든 모니터와 그래픽카드 조합을 저장할 리스트를 생성합니다.
	if (!DisplayModeList)
		return;

	if (FAILED(AdapterOutput->GetDisplayModeList(DXGI_FORMAT_R8G8B8A8_UNORM, DXGI_ENUM_MODES_INTERLACED, &NumModes, DisplayModeList)))		// 이제 디스플레이 모드에 대한 리스트를 채웁니다.
	{
		return;
	}

	// 이제 모든 디스플레이 모드에 대해 화면 너비/높이에 맞는 디스플레이 모드를 찾습니다. 적합한 것을 찾으면 모니터의 새로고침 비율의 분모와 분자 값을 저장합니다.
	unsigned int Numerator = 0;
	unsigned int Denominator = 0;
	for (unsigned int i = 0; i < NumModes; ++i)
	{
		const auto& CurDisplayMode = DisplayModeList[i];
		if ((CurDisplayMode.Width == (unsigned int)ScreenWidth) && (CurDisplayMode.Height == (unsigned int)ScreenHeight))
		{
			Numerator = CurDisplayMode.RefreshRate.Numerator;
			Denominator = CurDisplayMode.RefreshRate.Denominator;
		}
	}

	DXGI_ADAPTER_DESC AdapterDesc;
	if (FAILED(Adapter->GetDesc(&AdapterDesc)))		// 비디오카드의 구조체를 얻습니다.
		return;

	VideoCardMemory = (int)(AdapterDesc.DedicatedVideoMemory / (1024 * 1024));		// 비디오카드 메모리 용량 단위를 MB 단위로 저장합니다.

	size_t StrLen = 0;
	if (wcstombs_s(&StrLen, VideoCardDescription, sizeof(VideoCardDescription), AdapterDesc.Description, sizeof(AdapterDesc.Description)))		// 비디오카드 이름을 저장합니다.
		return;

	delete[] DisplayModeList;		// 디스플레이 모드 리스트를 해제합니다.
	DisplayModeList = nullptr;

	AdapterOutput->Release();		// 출력 어뎁터를 해제합니다.
	AdapterOutput = nullptr;

	Adapter->Release();				// 어댑터를 해제합니다.
	Adapter = nullptr;

	Factory->Release();				// 팩토리 객체를 해제합니다.
	Factory = nullptr;

	DXGI_SWAP_CHAIN_DESC SwapChainDesc;															// 스왑체인 구조체를 초기화합니다.
	ZeroMemory(&SwapChainDesc, sizeof(SwapChainDesc));

	SwapChainDesc.BufferCount = 1;																// 백버퍼 1개만 사용하도록 지정
	SwapChainDesc.BufferDesc.Width = ScreenWidth;
	SwapChainDesc.BufferDesc.Height = ScreenHeight;
	SwapChainDesc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;								// 32Bit 서페이스를 설정
	if (IsVsync)
	{
		SwapChainDesc.BufferDesc.RefreshRate.Numerator = Numerator;
		SwapChainDesc.BufferDesc.RefreshRate.Denominator = Denominator;
	}
	else
	{
		SwapChainDesc.BufferDesc.RefreshRate.Numerator = 0;
		SwapChainDesc.BufferDesc.RefreshRate.Denominator = 1;
	}
	SwapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	SwapChainDesc.OutputWindow = Hwnd;															// 렌더링에 사용될 윈도우 핸들 지정
	SwapChainDesc.SampleDesc.Count = 1;
	SwapChainDesc.SampleDesc.Quality = 0;
	SwapChainDesc.Windowed = !IsFullScreen;

	SwapChainDesc.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;			// 스캔라인 순서 및 크기를 지정하지 않음으로 설정
	SwapChainDesc.BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;

	SwapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;										// 출력된 다음 백버퍼를 비우도록 지정
	SwapChainDesc.Flags = 0;

	D3D_FEATURE_LEVEL FeatureLevel = D3D_FEATURE_LEVEL_11_0;									// 피쳐레벨을 DX11로 설정

	// 스왑체인, Direct3D 장치 그리고 Direct3D 장치 컨텍스트를 만듭니다.
	if (FAILED(D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, 0, &FeatureLevel, 1
		, D3D11_SDK_VERSION, &SwapChainDesc, &SwapChain, &Device, nullptr, &DeviceContext)))
	{
		return;
	}

	ID3D11Texture2D* Backbuffer = nullptr;
	if (FAILED(SwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (LPVOID*)&Backbuffer)))		// 백버퍼 포인터를 얻어옵니다.
		return;

	if (FAILED(Device->CreateRenderTargetView(Backbuffer, nullptr, &RenderTargetView)))			// 백버퍼 포인터로 렌더타겟 뷰를 생성
		return;

	Backbuffer->Release();																		// 백버퍼 포인터를 해제합니다.
	Backbuffer = nullptr;

	D3D11_TEXTURE2D_DESC DepthBufferDesc;														// 깊이버퍼 구조체를 초기화 합니다.
	ZeroMemory(&DepthBufferDesc, sizeof(DepthBufferDesc));

	DepthBufferDesc.Width = ScreenWidth;
	DepthBufferDesc.Height = ScreenHeight;
	DepthBufferDesc.MipLevels = 1;
	DepthBufferDesc.ArraySize = 1;
	DepthBufferDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
	DepthBufferDesc.SampleDesc.Count = 1;
	DepthBufferDesc.SampleDesc.Quality = 0;
	DepthBufferDesc.Usage = D3D11_USAGE_DEFAULT;
	DepthBufferDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL;
	DepthBufferDesc.CPUAccessFlags = 0;
	DepthBufferDesc.MiscFlags = 0;

	if (Device->CreateTexture2D(&DepthBufferDesc, nullptr, &DepthStencilBuffer))				// 깊이버퍼 텍스쳐 생성
		return;

	D3D11_DEPTH_STENCIL_DESC DepthStencilDesc;													// 스텐실상태 구조체를 초기화합니다.
	ZeroMemory(&DepthStencilDesc, sizeof(DepthStencilDesc));

	DepthStencilDesc.DepthEnable = true;
	DepthStencilDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
	DepthStencilDesc.DepthFunc = D3D11_COMPARISON_LESS;

	DepthStencilDesc.StencilEnable = true;
	DepthStencilDesc.StencilReadMask = 0xFF;
	DepthStencilDesc.StencilWriteMask = 0xFF;

	DepthStencilDesc.FrontFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
	DepthStencilDesc.FrontFace.StencilDepthFailOp = D3D11_STENCIL_OP_INCR;
	DepthStencilDesc.FrontFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
	DepthStencilDesc.FrontFace.StencilFunc = D3D11_COMPARISON_ALWAYS;

	DepthStencilDesc.BackFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
	DepthStencilDesc.BackFace.StencilDepthFailOp = D3D11_STENCIL_OP_DECR;
	DepthStencilDesc.BackFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
	DepthStencilDesc.BackFace.StencilFunc = D3D11_COMPARISON_ALWAYS;

	if (FAILED(Device->CreateDepthStencilState(&DepthStencilDesc, &DepthStencilState)))
		return;

	D3D11_DEPTH_STENCIL_VIEW_DESC DepthStencilViewDesc;
	ZeroMemory(&DepthStencilViewDesc, sizeof(DepthStencilViewDesc));
	DepthStencilViewDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
	DepthStencilViewDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
	DepthStencilViewDesc.Texture2D.MipSlice = 0;

	if (FAILED(Device->CreateDepthStencilView(DepthStencilBuffer, &DepthStencilViewDesc, &DepthStencilView)))		// 깊이 스텐실 뷰를 생성
		return;

	D3D11_RASTERIZER_DESC RasterDesc;
	RasterDesc.AntialiasedLineEnable = false;
	RasterDesc.CullMode = D3D11_CULL_BACK;
	RasterDesc.DepthBias = 0;
	RasterDesc.DepthBiasClamp = 0.0f;
	RasterDesc.DepthClipEnable = true;
	RasterDesc.FillMode = D3D11_FILL_SOLID;
	RasterDesc.FrontCounterClockwise = false;
	RasterDesc.MultisampleEnable = false;
	RasterDesc.ScissorEnable = false;
	RasterDesc.SlopeScaledDepthBias = 0.0f;

	if (FAILED(Device->CreateRasterizerState(&RasterDesc, &RasterState)))											// 레스터라이즈 상태 생성
		return;

	D3D11_VIEWPORT Viewport;																						// 뷰포트 정보 설정
	Viewport.Width = (float)ScreenWidth;
	Viewport.Height = (float)ScreenHeight;
	Viewport.MinDepth = 0.0f;
	Viewport.MaxDepth = 1.0f;
	Viewport.TopLeftX = 0.0f;
	Viewport.TopLeftY = 0.0f;

	//////////////////////////////////////////////////////////////////////////
	// 4. 지오메트리 준비
	//////////////////////////////////////////////////////////////////////////
	const int VertexCount = 3;
	const int IndexCount = 3;

	struct VertexType
	{
		XMFLOAT3 Position;
		XMFLOAT4 Color;
	};

	VertexType* Vertices = new VertexType[VertexCount];
	if (!Vertices)
		return;

	unsigned long* Indices = new unsigned long[IndexCount];
	if (!Indices)
		return;

	// 정점 배열에 데이터를 설정합니다.
	Vertices[0].Position = XMFLOAT3(-1.0f, -1.0f, 0.0f);  // Bottom left.
	Vertices[0].Color = XMFLOAT4(0.0f, 1.0f, 0.0f, 1.0f);

	Vertices[1].Position = XMFLOAT3(0.0f, 1.0f, 0.0f);  // Top middle.
	Vertices[1].Color = XMFLOAT4(0.0f, 1.0f, 0.0f, 1.0f);

	Vertices[2].Position = XMFLOAT3(1.0f, -1.0f, 0.0f);  // Bottom right.
	Vertices[2].Color = XMFLOAT4(0.0f, 1.0f, 0.0f, 1.0f);

	Indices[0] = 0;
	Indices[1] = 1;
	Indices[2] = 2;

	D3D11_BUFFER_DESC VertexBufferDesc;
	VertexBufferDesc.Usage = D3D11_USAGE_DEFAULT;
	VertexBufferDesc.ByteWidth = sizeof(VertexType) * VertexCount;
	VertexBufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	VertexBufferDesc.CPUAccessFlags = 0;
	VertexBufferDesc.MiscFlags = 0;
	VertexBufferDesc.StructureByteStride = 0;

	// Subresource 구조에 정점 데이터에 대한 포인터 제공
	D3D11_SUBRESOURCE_DATA VertexData;
	VertexData.pSysMem = Vertices;
	VertexData.SysMemPitch = 0;
	VertexData.SysMemSlicePitch = 0;

	if (FAILED(Device->CreateBuffer(&VertexBufferDesc, &VertexData, &VertexBuffer)))
		return;

	D3D11_BUFFER_DESC IndexBufferDesc;
	IndexBufferDesc.Usage = D3D11_USAGE_DEFAULT;
	IndexBufferDesc.ByteWidth = sizeof(unsigned long) * IndexCount;
	IndexBufferDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
	IndexBufferDesc.CPUAccessFlags = 0;
	IndexBufferDesc.MiscFlags = 0;
	IndexBufferDesc.StructureByteStride = 0;

	// Subresource 구조에 인덱스 데이터에 대한 포인터 제공
	D3D11_SUBRESOURCE_DATA IndexData;
	IndexData.pSysMem = Indices;
	IndexData.SysMemPitch = 0;
	IndexData.SysMemSlicePitch = 0;

	if (FAILED(Device->CreateBuffer(&IndexBufferDesc, &IndexData, &IndexBuffer)))
		return;

	//////////////////////////////////////////////////////////////////////////
	// 5 . 쉐이더, 버택스 레이아웃, 상수 버퍼 생성
	//////////////////////////////////////////////////////////////////////////
	ID3D10Blob* ErrorMessage = nullptr;

	ID3D10Blob* VertexShaderBuffer = nullptr;
	if (FAILED(D3DCompile(VertexShaderString, sizeof(VertexShaderString), nullptr, nullptr, nullptr, "ColorVertexShader", "vs_5_0"
		, D3D10_SHADER_ENABLE_STRICTNESS, 0, &VertexShaderBuffer, &ErrorMessage)))
	{
		OutputDebugStringA(reinterpret_cast<const char*>(ErrorMessage->GetBufferPointer()));
		return;
	}

	ID3D10Blob* PixelShaderBuffer = nullptr;
		if (FAILED(D3DCompile(PixelShaderString, sizeof(PixelShaderString), nullptr, nullptr, nullptr, "ColorPixelShader", "ps_5_0"
			, D3D10_SHADER_ENABLE_STRICTNESS, 0, &PixelShaderBuffer, &ErrorMessage)))
	{
		OutputDebugStringA(reinterpret_cast<const char*>(ErrorMessage->GetBufferPointer()));
		return;
	}

	if (FAILED(Device->CreateVertexShader(VertexShaderBuffer->GetBufferPointer(), VertexShaderBuffer->GetBufferSize(), nullptr, &VertexShader)))
		return;

	if (FAILED(Device->CreatePixelShader(PixelShaderBuffer->GetBufferPointer(), PixelShaderBuffer->GetBufferSize(), nullptr, &PixelShader)))
		return;

	// 정점 입력 레이아웃 구조체 설정
	// 이 설정은 ModelClass와 쉐이더의 VertexType 구조와 일치해야 합니다.
	D3D11_INPUT_ELEMENT_DESC PolygonLayout[2];
	PolygonLayout[0].SemanticName = "POSITION";
	PolygonLayout[0].SemanticIndex = 0;
	PolygonLayout[0].Format = DXGI_FORMAT_R32G32B32_FLOAT;
	PolygonLayout[0].InputSlot = 0;
	PolygonLayout[0].AlignedByteOffset = 0;
	PolygonLayout[0].InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;
	PolygonLayout[0].InstanceDataStepRate = 0;

	PolygonLayout[1].SemanticName = "COLOR";
	PolygonLayout[1].SemanticIndex = 0;
	PolygonLayout[1].Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
	PolygonLayout[1].InputSlot = 0;
	PolygonLayout[1].AlignedByteOffset = D3D11_APPEND_ALIGNED_ELEMENT;
	PolygonLayout[1].InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;
	PolygonLayout[1].InstanceDataStepRate = 0;

	unsigned int NumElements = sizeof(PolygonLayout) / sizeof(PolygonLayout[0]);

	// 정점 입력 레이아웃을 만듬
	if (FAILED(Device->CreateInputLayout(PolygonLayout, NumElements, VertexShaderBuffer->GetBufferPointer(), VertexShaderBuffer->GetBufferSize(), &Layout)))
		return;

	DX_RELEASE(VertexShaderBuffer);			// 더이상 사용하지 않는 버퍼들 제거
	DX_RELEASE(PixelShaderBuffer);

	struct MatrixBufferType
	{
		XMMATRIX World;
		XMMATRIX View;
		XMMATRIX Projection;
	};

	// 정점 쉐이더에 있는 행렬 상수 버퍼의 구조체 작성
	D3D11_BUFFER_DESC MatrixBufferDesc;
	MatrixBufferDesc.Usage = D3D11_USAGE_DYNAMIC;
	MatrixBufferDesc.ByteWidth = sizeof(MatrixBufferType);
	MatrixBufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	MatrixBufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	MatrixBufferDesc.MiscFlags = 0;
	MatrixBufferDesc.StructureByteStride = 0;

	if (FAILED(Device->CreateBuffer(&MatrixBufferDesc, nullptr, &MatrixBuffer)))
		return;

	MSG msg;
	while (true)
	{
		if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
		{
			if (msg.message == WM_QUIT)
				break;

			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
		else
		{
            //////////////////////////////////////////////////////////////////////////
            // 6 . Render loop
            //////////////////////////////////////////////////////////////////////////

			//-----------------------------------------------------------------------
			// 상수 버퍼 갱신 - 카메라 매트릭스 관련 정보들 설정
			//-----------------------------------------------------------------------
            const float FieldOfView = 3.14f / 4.0f;
            const float ScreenAspect = (float)ScreenWidth / (float)ScreenHeight;

            const float DepthNear = 0.1f;
            const float DepthFar = 1000.0f;
            const XMMATRIX ProjectionMatrix = XMMatrixPerspectiveFovLH(FieldOfView, ScreenAspect, DepthNear, DepthFar);
            const XMMATRIX WorldMatrix = XMMatrixIdentity();
            const XMMATRIX OrthoMatrix = XMMatrixOrthographicLH((float)ScreenWidth, (float)ScreenHeight, DepthNear, DepthFar);

			const XMFLOAT3 Pos = { 0.0f, 0.0f, -5.0f };
			const XMFLOAT3 Up = { 0.0f, 1.0f, 0.0f };
			const XMFLOAT3 LookAt = { 0.0f, 0.0f, 1.0f };
			const XMFLOAT3 Rotation = { 0.0f, 0.0f, 0.0f };

			XMVECTOR UpVector, PositionVector, LookAtVector;
			float Yaw, Pitch, Roll;			

			UpVector = XMLoadFloat3(&Up);
			PositionVector = XMLoadFloat3(&Pos);
			LookAtVector = XMLoadFloat3(&LookAt);

			static float temp = 0.0174532925f;
			Pitch = Rotation.x * temp;
			Yaw = Rotation.y * temp;
			Roll = Rotation.z * temp;

			const XMMATRIX RotationMatrix = XMMatrixRotationRollPitchYaw(Pitch, Yaw, Roll);

			LookAtVector = XMVector3TransformCoord(LookAtVector, RotationMatrix);
			UpVector = XMVector3TransformCoord(UpVector, RotationMatrix);
			LookAtVector = XMVectorAdd(PositionVector, LookAtVector);
			auto ViewMatrix = XMMatrixLookAtLH(PositionVector, LookAtVector, UpVector);

			D3D11_MAPPED_SUBRESOURCE MappedResource;
			if (FAILED(DeviceContext->Map(MatrixBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &MappedResource)))
				return;

			MatrixBufferType* DataPtr = (MatrixBufferType*)MappedResource.pData;

			// 행렬을 Transpose 하여 쉐이더에서 사용할 수 있게 합니다.
			DataPtr->World = XMMatrixTranspose(WorldMatrix);
			DataPtr->View = XMMatrixTranspose(ViewMatrix);
			DataPtr->Projection = XMMatrixTranspose(ProjectionMatrix);

			DeviceContext->Unmap(MatrixBuffer, 0);
			//-----------------------------------------------------------------------

            unsigned int Stride = sizeof(VertexType);
            unsigned int Offset = 0;

            unsigned int BufferNumber = 0;
            DeviceContext->VSSetConstantBuffers(BufferNumber, 1, &MatrixBuffer);					// 상수 버퍼 설정

			// 렌더타겟 바인딩
            DeviceContext->OMSetRenderTargets(1, &RenderTargetView, DepthStencilView);
            DeviceContext->OMSetDepthStencilState(DepthStencilState, 1);

			// 렌더타겟 클리어
            const float ClearColor[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
            DeviceContext->ClearRenderTargetView(RenderTargetView, ClearColor);
            DeviceContext->ClearDepthStencilView(DepthStencilView, D3D11_CLEAR_DEPTH, 1.0f, 0);

            DeviceContext->RSSetViewports(1, &Viewport);											// 뷰포트 설정
			DeviceContext->RSSetState(RasterState);													// 레스터 라이즈 상태 설정

            DeviceContext->IASetVertexBuffers(0, 1, &VertexBuffer, &Stride, &Offset);				// 지오메트리 버퍼들 바인딩 및 프리미티브 형식 설정
            DeviceContext->IASetIndexBuffer(IndexBuffer, DXGI_FORMAT_R32_UINT, 0);
            DeviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
			DeviceContext->IASetInputLayout(Layout);												// 버택스 인풋 레이아웃 설정
			
			DeviceContext->VSSetShader(VertexShader, nullptr, 0);									// 쉐이더 설정
			DeviceContext->PSSetShader(PixelShader, nullptr, 0);

			DeviceContext->DrawIndexed(IndexCount, 0, 0);											// 렌더링

			SwapChain->Present(IsVsync ? 1 : 0, 0);													// 결과 출력
		}
	}
}

LRESULT CALLBACK SystemClassWndProc(HWND hwnd, UINT umessage, WPARAM wparam, LPARAM lparam)
{
	switch (umessage)
	{
		case WM_DESTROY:
		{
			PostQuitMessage(0);
			return 0;
		}
		case WM_CLOSE:
		{
			PostQuitMessage(0);
			return 0;
		}
		default:
		{
			return DefWindowProc(hwnd, umessage, wparam, lparam);
		}
	}
}