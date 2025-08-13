// 참고: 헤더 include 순서
// https://google.github.io/styleguide/cppguide.html#Names_and_Order_of_Includes

#include "AppBase.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include <directxtk/DDSTextureLoader.h> // 큐브맵 읽을 때 필요
#include <dxgi.h>                       // DXGIFactory
#include <dxgi1_4.h>                    // DXGIFactory4

// imgui_impl_win32.cpp에 정의된 메시지 처리 함수에 대한 전방 선언
// VCPKG를 통해 IMGUI를 사용할 경우 빨간줄로 경고가 뜰 수 있음
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd,
                                                             UINT msg,
                                                             WPARAM wParam,
                                                             LPARAM lParam);

namespace hlab {

using namespace std;
using namespace DirectX;

// RegisterClassEx()에서 멤버 함수를 직접 등록할 수가 없기 때문에
// 클래스의 멤버 함수에서 간접적으로 메시지를 처리할 수 있도록 도와줍니다.
AppBase *g_appBase = nullptr;

// RegisterClassEx()에서 실제로 등록될 콜백 함수
LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {

    // g_appBase를 이용해서 간접적으로 멤버 함수 호출
    return g_appBase->MsgProc(hWnd, msg, wParam, lParam);
}

// 생성자
AppBase::AppBase()
    : m_screenWidth(1280), m_screenHeight(960), m_mainWindow(0),
      m_screenViewport(D3D11_VIEWPORT()) {

    g_appBase = this;
}

AppBase::~AppBase() {
    g_appBase = nullptr;

    // Cleanup
    ImGui_ImplDX11_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();

    DestroyWindow(m_mainWindow);
    // UnregisterClass(wc.lpszClassName, wc.hInstance);//생략

    // COMPtr에서 알아서 release
    // ComPtr automatically maintains a reference count for the underlying
    // interface pointer and releases the interface when the reference count
    // goes to zero.
    // https:learn.microsoft.com/en-us/cpp/cppcx/wrl/comptr-class?view=msvc-170
    // 예시: m_d3dDevice.Reset(); 생략
}

float AppBase::GetAspectRatio() const {
    return float(m_screenWidth - m_guiWidth) / m_screenHeight;
}

int AppBase::Run() {

    // Main message loop
    MSG msg = {0};

    bool showGui = true;
    while (WM_QUIT != msg.message) {
        if (GetAsyncKeyState(VK_F1) & 1) {
            showGui = !showGui;
        }

        if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        } else {

            ImGui_ImplDX11_NewFrame(); // GUI 프레임 시작
            ImGui_ImplWin32_NewFrame();

            ImGui::NewFrame(); // 어떤 것들을 렌더링 할지 기록 시작

            if (showGui) {
                ImGui::Begin("Scene Control");

                // ImGui가 측정해주는 Framerate 출력
                ImGui::Text("Average %.3f ms/frame (%.1f FPS)",
                            1000.0f / ImGui::GetIO().Framerate,
                            ImGui::GetIO().Framerate);

                UpdateGUI(); // 추가적으로 사용할 GUI

                ImGui::SetWindowPos(ImVec2(0.0f, 0.0f));

                m_guiWidth = int(ImGui::GetWindowWidth());

                ImGui::End();
            }

            ImGui::Render(); // 렌더링할 것들 기록 끝

            Update(ImGui::GetIO().DeltaTime); // 애니메이션 같은 변화

            Render(); // 우리가 구현한 렌더링

            ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData()); // GUI 렌더링

            // Switch the back buffer and the front buffer
            // 주의: ImGui RenderDrawData() 다음에 Present() 호출
            m_swapChain->Present(1, 0);
        }
    }

    return 0;
}

bool AppBase::Initialize() {

    if (!InitMainWindow())
        return false;

    if (!InitDirect3D())
        return false;

    if (!InitGUI())
        return false;

    return true;
}

LRESULT AppBase::MsgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {

    if (ImGui_ImplWin32_WndProcHandler(hwnd, msg, wParam, lParam))
        return true;

    switch (msg) {
    case WM_SIZE:
        // Reset and resize swapchain
        // std::cout << (UINT)LOWORD(lParam) << " " << (UINT)HIWORD(lParam)
        //          << std::endl;

        if (m_swapChain) { // 처음 실행이 아닌지 확인

            m_screenWidth = int(LOWORD(lParam));
            m_screenHeight = int(HIWORD(lParam));
            m_guiWidth = 0;

            m_renderTargetView.Reset();
            m_swapChain->ResizeBuffers(0,                    // 현재 개수 유지
                                       (UINT)LOWORD(lParam), // 해상도 변경
                                       (UINT)HIWORD(lParam),
                                       DXGI_FORMAT_UNKNOWN, // 현재 포맷 유지
                                       0);
            CreateRenderTargetView();
            CreateDepthBuffer();
            SetViewport();
        }

        break;
    case WM_SYSCOMMAND:
        if ((wParam & 0xfff0) == SC_KEYMENU) // Disable ALT application menu
            return 0;
        break;
    case WM_MOUSEMOVE:
        // cout << "Mouse " << LOWORD(lParam) << " " << HIWORD(lParam) << endl;
        break;
    case WM_LBUTTONUP:
        // cout << "WM_LBUTTONUP Left mouse button" << endl;
        break;
    case WM_RBUTTONUP:
        // cout << "WM_RBUTTONUP Right mouse button" << endl;
        break;
    case WM_KEYDOWN:
        // cout << "WM_KEYDOWN " << (int)wParam << endl;
        break;
    case WM_DESTROY:
        ::PostQuitMessage(0);
        return 0;
    }

    return ::DefWindowProc(hwnd, msg, wParam, lParam);
}

bool AppBase::InitMainWindow() {

    WNDCLASSEX wc = {sizeof(WNDCLASSEX),
                     CS_CLASSDC,
                     WndProc,
                     0L,
                     0L,
                     GetModuleHandle(NULL),
                     NULL,
                     NULL,
                     NULL,
                     NULL,
                     L"HongLabGraphics", // lpszClassName, L-string
                     NULL};

    // The RegisterClass function has been superseded by the RegisterClassEx
    // function.
    // https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-registerclassa?redirectedfrom=MSDN
    if (!RegisterClassEx(&wc)) {
        cout << "RegisterClassEx() failed." << endl;
        return false;
    }

    // 툴바까지 포함한 윈도우 전체 해상도가 아니라
    // 우리가 실제로 그리는 해상도가 width x height가 되도록
    // 윈도우를 만들 해상도를 다시 계산해서 CreateWindow()에서 사용

    // 우리가 원하는 그림이 그려질 부분의 해상도
    RECT wr = {0, 0, m_screenWidth, m_screenHeight};

    // 필요한 윈도우 크기(해상도) 계산
    // wr의 값이 바뀜
    AdjustWindowRect(&wr, WS_OVERLAPPEDWINDOW, false);

    // 윈도우를 만들때 위에서 계산한 wr 사용
    m_mainWindow = CreateWindow(wc.lpszClassName, L"HongLabGraphics Example",
                                WS_OVERLAPPEDWINDOW,
                                100,                // 윈도우 좌측 상단의 x 좌표
                                100,                // 윈도우 좌측 상단의 y 좌표
                                wr.right - wr.left, // 윈도우 가로 방향 해상도
                                wr.bottom - wr.top, // 윈도우 세로 방향 해상도
                                NULL, NULL, wc.hInstance, NULL);

    if (!m_mainWindow) {
        cout << "CreateWindow() failed." << endl;
        return false;
    }

    ShowWindow(m_mainWindow, SW_SHOWDEFAULT);
    UpdateWindow(m_mainWindow);

    return true;
}

bool AppBase::InitDirect3D() {

    // 이 예제는 Intel 내장 그래픽스 칩으로 실행을 확인하였습니다.
    // (LG 그램, 17Z90n, Intel Iris Plus Graphics)
    // 만약 그래픽스 카드 호환성 문제로 D3D11CreateDevice()가 실패하는 경우에는
    // D3D_DRIVER_TYPE_HARDWARE 대신 D3D_DRIVER_TYPE_WARP 사용해보세요
    // const D3D_DRIVER_TYPE driverType = D3D_DRIVER_TYPE_WARP;
    const D3D_DRIVER_TYPE driverType = D3D_DRIVER_TYPE_HARDWARE;

    // 여기서 생성하는 것들
    // m_device, m_context, m_swapChain,
    // m_renderTargetView, m_screenViewport, m_rasterizerSate

    // m_device와 m_context 생성

    UINT createDeviceFlags = 0;
#if defined(DEBUG) || defined(_DEBUG)
    createDeviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

    ComPtr<ID3D11Device> device;
    ComPtr<ID3D11DeviceContext> context;

    const D3D_FEATURE_LEVEL featureLevels[2] = {
        D3D_FEATURE_LEVEL_11_0, // 더 높은 버전이 먼저 오도록 설정
        D3D_FEATURE_LEVEL_9_3};
    D3D_FEATURE_LEVEL featureLevel;

    if (FAILED(D3D11CreateDevice(
            nullptr,    // Specify nullptr to use the default adapter.
            driverType, // Create a device using the hardware graphics driver.
            0, // Should be 0 unless the driver is D3D_DRIVER_TYPE_SOFTWARE.
            createDeviceFlags, // Set debug and Direct2D compatibility flags.
            featureLevels,     // List of feature levels this app can support.
            ARRAYSIZE(featureLevels), // Size of the list above.
            D3D11_SDK_VERSION,     // Always set this to D3D11_SDK_VERSION for
                                   // Microsoft Store apps.
            device.GetAddressOf(), // Returns the Direct3D device created.
            &featureLevel,         // Returns feature level of device created.
            context.GetAddressOf() // Returns the device immediate context.
            ))) {
        cout << "D3D11CreateDevice() failed." << endl;
        return false;
    }

    /* 참고: 오류가 있을 경우 예외 발생 방법

    // MS 예제
    inline void ThrowIfFailed(HRESULT hr)
    {
        if (FAILED(hr))
        {
            // Set a breakpoint on this line to catch Win32 API errors.
            throw Platform::Exception::CreateException(hr);
        }
    }

    // Luna DX12 교재
    #ifndef ThrowIfFailed
    #define ThrowIfFailed(x)                                              \
    {                                                                     \
        HRESULT hr__ = (x);                                               \
        std::wstring wfn = AnsiToWString(__FILE__);                       \
        if(FAILED(hr__)) { throw DxException(hr__, L#x, wfn, __LINE__); } \
    }
    #endif
    */

    if (featureLevel != D3D_FEATURE_LEVEL_11_0) {
        cout << "D3D Feature Level 11 unsupported." << endl;
        return false;
    }

    // 참고: Immediate vs deferred context
    // A deferred context is primarily used for multithreading and is not
    // necessary for a single-threaded application.
    // https://learn.microsoft.com/en-us/windows/win32/direct3d11/overviews-direct3d-11-devices-intro#deferred-context

    // 4X MSAA 지원하는지 확인
    // UINT numQualityLevels; // Resize 처리를 위해 멤버 변수로 변경
    device->CheckMultisampleQualityLevels(DXGI_FORMAT_R8G8B8A8_UNORM, 4,
                                          &numQualityLevels);
    if (numQualityLevels <= 0) {
        cout << "MSAA not supported." << endl;
    }

    // numQualityLevels = 0; // MSAA 강제로 끄기

    if (FAILED(device.As(&m_device))) {
        cout << "device.AS() failed." << endl;
        return false;
    }

    if (FAILED(context.As(&m_context))) {
        cout << "context.As() failed." << endl;
        return false;
    }

    DXGI_SWAP_CHAIN_DESC sd;
    ZeroMemory(&sd, sizeof(sd));
    sd.BufferDesc.Width = m_screenWidth;   // set the back buffer width
    sd.BufferDesc.Height = m_screenHeight; // set the back buffer height
    sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM; // use 32-bit color
    sd.BufferCount = 2;                                // Double-buffering
    sd.BufferDesc.RefreshRate.Numerator = 60;
    sd.BufferDesc.RefreshRate.Denominator = 1;
    sd.BufferUsage =
        DXGI_USAGE_RENDER_TARGET_OUTPUT; // how swap chain is to be used
    sd.OutputWindow = m_mainWindow;      // the window to be used
    sd.Windowed = TRUE;                  // windowed/full-screen mode
    sd.Flags =
        DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH; // allow full-screen switching
    sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
    if (numQualityLevels > 0) {
        sd.SampleDesc.Count = 4; // how many multisamples
        sd.SampleDesc.Quality = numQualityLevels - 1;
    } else {
        sd.SampleDesc.Count = 1; // how many multisamples
        sd.SampleDesc.Quality = 0;
    }

    if (FAILED(D3D11CreateDeviceAndSwapChain(
            0, // Default adapter
            driverType,
            0, // No software device
            createDeviceFlags, featureLevels, 1, D3D11_SDK_VERSION, &sd,
            m_swapChain.GetAddressOf(), m_device.GetAddressOf(), &featureLevel,
            m_context.GetAddressOf()))) {
        cout << "D3D11CreateDeviceAndSwapChain() failed." << endl;
        return false;
    }

    // 참고: IDXGIFactory를 이용한 CreateSwapChain()
    /*
    ComPtr<IDXGIDevice3> dxgiDevice;
    m_device.As(&dxgiDevice);

    ComPtr<IDXGIAdapter> dxgiAdapter;
    dxgiDevice->GetAdapter(&dxgiAdapter);

    ComPtr<IDXGIFactory> dxgiFactory;
    dxgiAdapter->GetParent(IID_PPV_ARGS(&dxgiFactory));

    ComPtr<IDXGISwapChain> swapChain;
    dxgiFactory->CreateSwapChain(m_device.Get(), &sd, &swapChain);

    swapChain.As(&m_swapChain);
    */

    // 참고: IDXGIFactory4를 이용한 CreateSwapChainForHwnd()
    /*
    ComPtr<IDXGIFactory4> dxgiFactory;
    dxgiAdapter->GetParent(IID_PPV_ARGS(&dxgiFactory));

    DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {0};
    swapChainDesc.Width = lround(m_screenWidth); // Match the size of the
    window. swapChainDesc.Height = lround(m_screenHeight); swapChainDesc.Format
    = DXGI_FORMAT_B8G8R8A8_UNORM; // This is the most common swap chain format.
    swapChainDesc.Stereo = false;
    swapChainDesc.SampleDesc.Count = 1; // Don't use multi-sampling.
    swapChainDesc.SampleDesc.Quality = 0;
    swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swapChainDesc.BufferCount = 2; // Use double-buffering to minimize latency.
    swapChainDesc.SwapEffect =
        DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL; // All Microsoft Store apps must use
    this SwapEffect. swapChainDesc.Flags = 0; swapChainDesc.Scaling =
    DXGI_SCALING_NONE; swapChainDesc.AlphaMode = DXGI_ALPHA_MODE_IGNORE;

    ComPtr<IDXGISwapChain1> swapChain;
    dxgiFactory->CreateSwapChainForHwnd(m_device.Get(), m_mainWindow,
    &swapChainDesc, nullptr, nullptr, swapChain.GetAddressOf());
    */

    CreateRenderTargetView();

    SetViewport();

    // Create a rasterizer state
    D3D11_RASTERIZER_DESC rastDesc;
    ZeroMemory(&rastDesc, sizeof(D3D11_RASTERIZER_DESC)); // Need this
    rastDesc.FillMode = D3D11_FILL_MODE::D3D11_FILL_SOLID;
    // rastDesc.FillMode = D3D11_FILL_MODE::D3D11_FILL_WIREFRAME;
    rastDesc.CullMode = D3D11_CULL_MODE::D3D11_CULL_NONE;
    rastDesc.FrontCounterClockwise = false;
    rastDesc.DepthClipEnable = true; // <- zNear, zFar 확인에 필요

    m_device->CreateRasterizerState(&rastDesc,
                                    m_solidRasterizerSate.GetAddressOf());

    rastDesc.FillMode = D3D11_FILL_MODE::D3D11_FILL_WIREFRAME;

    m_device->CreateRasterizerState(&rastDesc,
                                    m_wireRasterizerSate.GetAddressOf());

    CreateDepthBuffer();

    // Create depth stencil state
    D3D11_DEPTH_STENCIL_DESC depthStencilDesc;
    ZeroMemory(&depthStencilDesc, sizeof(D3D11_DEPTH_STENCIL_DESC));
    depthStencilDesc.DepthEnable = true; // false
    depthStencilDesc.DepthWriteMask =
        D3D11_DEPTH_WRITE_MASK::D3D11_DEPTH_WRITE_MASK_ALL;
    depthStencilDesc.DepthFunc =
        D3D11_COMPARISON_FUNC::D3D11_COMPARISON_LESS_EQUAL;
    if (FAILED(m_device->CreateDepthStencilState(
            &depthStencilDesc, m_depthStencilState.GetAddressOf()))) {
        cout << "CreateDepthStencilState() failed." << endl;
    }

    return true;
}

bool AppBase::InitGUI() {

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO &io = ImGui::GetIO();
    (void)io;
    io.DisplaySize = ImVec2(float(m_screenWidth), float(m_screenHeight));
    ImGui::StyleColorsLight();

    // Setup Platform/Renderer backends
    if (!ImGui_ImplDX11_Init(m_device.Get(), m_context.Get())) {
        return false;
    }

    if (!ImGui_ImplWin32_Init(m_mainWindow)) {
        return false;
    }

    return true;
}

void AppBase::SetViewport() {

    static int previousGuiWidth = m_guiWidth;

    if (previousGuiWidth != m_guiWidth) {

        previousGuiWidth = m_guiWidth;

        // Set the viewport
        ZeroMemory(&m_screenViewport, sizeof(D3D11_VIEWPORT));
        m_screenViewport.TopLeftX = float(m_guiWidth);
        m_screenViewport.TopLeftY = 0;
        m_screenViewport.Width = float(m_screenWidth - m_guiWidth);
        m_screenViewport.Height = float(m_screenHeight);
        // m_screenViewport.Width = static_cast<float>(m_screenHeight);
        m_screenViewport.MinDepth = 0.0f;
        m_screenViewport.MaxDepth = 1.0f; // Note: important for depth buffering

        m_context->RSSetViewports(1, &m_screenViewport);
    }
}

bool AppBase::CreateRenderTargetView() {

    ComPtr<ID3D11Texture2D> backBuffer;
    m_swapChain->GetBuffer(0, IID_PPV_ARGS(backBuffer.GetAddressOf()));
    if (backBuffer) {
        m_device->CreateRenderTargetView(backBuffer.Get(), NULL,
                                         m_renderTargetView.GetAddressOf());
    } else {
        std::cout << "CreateRenderTargetView() failed." << std::endl;
        return false;
    }

    return true;
}

bool AppBase::CreateDepthBuffer() {
    D3D11_TEXTURE2D_DESC depthStencilBufferDesc;
    depthStencilBufferDesc.Width = m_screenWidth;
    depthStencilBufferDesc.Height = m_screenHeight;
    depthStencilBufferDesc.MipLevels = 1;
    depthStencilBufferDesc.ArraySize = 1;
    depthStencilBufferDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
    if (numQualityLevels > 0) {
        depthStencilBufferDesc.SampleDesc.Count = 4; // how many multisamples
        depthStencilBufferDesc.SampleDesc.Quality = numQualityLevels - 1;
    } else {
        depthStencilBufferDesc.SampleDesc.Count = 1; // how many multisamples
        depthStencilBufferDesc.SampleDesc.Quality = 0;
    }
    depthStencilBufferDesc.Usage = D3D11_USAGE_DEFAULT;
    depthStencilBufferDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL;
    depthStencilBufferDesc.CPUAccessFlags = 0;
    depthStencilBufferDesc.MiscFlags = 0;

    if (FAILED(m_device->CreateTexture2D(
            &depthStencilBufferDesc, 0, m_depthStencilBuffer.GetAddressOf()))) {
        std::cout << "CreateTexture2D() failed." << std::endl;
    }
    if (FAILED(m_device->CreateDepthStencilView(m_depthStencilBuffer.Get(), 0,
                                                &m_depthStencilView))) {
        std::cout << "CreateDepthStencilView() failed." << std::endl;
    }
    return true;
}

// 참고: How To: Compile a Shader
// https://docs.microsoft.com/en-us/windows/win32/direct3d11/how-to--compile-a-shader

// 참고: 앞에 L이 붙은 문자열은 wide character로 이루어진 문자열을
// 의미합니다.
// String and character literals (C++)
// https://learn.microsoft.com/en-us/cpp/cpp/string-and-character-literals-cpp?view=msvc-170

// 참고: 쉐이더를 미리 컴파일해둔 .cso 파일로부터 만들 수도 있습니다.
// 확장자 cso는 Compiled Shader Object를 의미합니다.
// 여기서는 쉐이더 파일을 읽어들여서 컴파일합니다.

void CheckResult(HRESULT hr, ID3DBlob *errorBlob) {
    if (FAILED(hr)) {
        // 파일이 없을 경우
        if ((hr & D3D11_ERROR_FILE_NOT_FOUND) != 0) {
            cout << "File not found." << endl;
        }

        // 에러 메시지가 있으면 출력
        if (errorBlob) {
            cout << "Shader compile error\n"
                 << (char *)errorBlob->GetBufferPointer() << endl;
        }
    }
}

void AppBase::CreateVertexShaderAndInputLayout(
    const wstring &filename,
    const vector<D3D11_INPUT_ELEMENT_DESC> &inputElements,
    ComPtr<ID3D11VertexShader> &vertexShader,
    ComPtr<ID3D11InputLayout> &inputLayout) {

    ComPtr<ID3DBlob> shaderBlob;
    ComPtr<ID3DBlob> errorBlob;

    UINT compileFlags = 0;
#if defined(DEBUG) || defined(_DEBUG)
    compileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#endif

    // 쉐이더의 시작점의 이름이 "main"인 함수로 지정
    // D3D_COMPILE_STANDARD_FILE_INCLUDE 추가: 쉐이더에서 include 사용
    HRESULT hr = D3DCompileFromFile(
        filename.c_str(), 0, D3D_COMPILE_STANDARD_FILE_INCLUDE, "main",
        "vs_5_0", compileFlags, 0, &shaderBlob, &errorBlob);

    CheckResult(hr, errorBlob.Get());

    m_device->CreateVertexShader(shaderBlob->GetBufferPointer(),
                                 shaderBlob->GetBufferSize(), NULL,
                                 &vertexShader);

    m_device->CreateInputLayout(inputElements.data(),
                                UINT(inputElements.size()),
                                shaderBlob->GetBufferPointer(),
                                shaderBlob->GetBufferSize(), &inputLayout);
}

void AppBase::CreatePixelShader(const wstring &filename,
                                ComPtr<ID3D11PixelShader> &pixelShader) {
    ComPtr<ID3DBlob> shaderBlob;
    ComPtr<ID3DBlob> errorBlob;

    UINT compileFlags = 0;
#if defined(DEBUG) || defined(_DEBUG)
    compileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#endif

    // 쉐이더의 시작점의 이름이 "main"인 함수로 지정
    // D3D_COMPILE_STANDARD_FILE_INCLUDE 추가: 쉐이더에서 include 사용
    HRESULT hr = D3DCompileFromFile(
        filename.c_str(), 0, D3D_COMPILE_STANDARD_FILE_INCLUDE, "main",
        "ps_5_0", compileFlags, 0, &shaderBlob, &errorBlob);

    CheckResult(hr, errorBlob.Get());

    m_device->CreatePixelShader(shaderBlob->GetBufferPointer(),
                                shaderBlob->GetBufferSize(), NULL,
                                &pixelShader);
}

void AppBase::CreateIndexBuffer(const std::vector<uint32_t> &indices,
                                ComPtr<ID3D11Buffer> &indexBuffer) {
    D3D11_BUFFER_DESC bufferDesc = {};
    bufferDesc.Usage = D3D11_USAGE_IMMUTABLE; // 초기화 후 변경X
    bufferDesc.ByteWidth = UINT(sizeof(uint32_t) * indices.size());
    bufferDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
    bufferDesc.CPUAccessFlags = 0; // 0 if no CPU access is necessary.
    bufferDesc.StructureByteStride = sizeof(uint32_t);

    D3D11_SUBRESOURCE_DATA indexBufferData = {0};
    indexBufferData.pSysMem = indices.data();
    indexBufferData.SysMemPitch = 0;
    indexBufferData.SysMemSlicePitch = 0;

    m_device->CreateBuffer(&bufferDesc, &indexBufferData,
                           indexBuffer.GetAddressOf());
}

void AppBase::CreateTexture(
    const std::string filename, ComPtr<ID3D11Texture2D> &texture,
    ComPtr<ID3D11ShaderResourceView> &textureResourceView) {

    int width, height, channels;

    unsigned char *img =
        stbi_load(filename.c_str(), &width, &height, &channels, 0);

    // assert(channels == 4);

    // 4채널로 만들어서 복사
    std::vector<uint8_t> image;
    image.resize(width * height * 4);
    for (size_t i = 0; i < width * height; i++) {
        for (size_t c = 0; c < 3; c++) {
            image[4 * i + c] = img[i * channels + c];
        }
        image[4 * i + 3] = 255;
    }

    // Create texture.
    D3D11_TEXTURE2D_DESC txtDesc = {};
    txtDesc.Width = width;
    txtDesc.Height = height;
    txtDesc.MipLevels = txtDesc.ArraySize = 1;
    txtDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    txtDesc.SampleDesc.Count = 1;
    txtDesc.Usage = D3D11_USAGE_IMMUTABLE;
    txtDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;

    // Fill in the subresource data.
    D3D11_SUBRESOURCE_DATA InitData;
    InitData.pSysMem = image.data();
    InitData.SysMemPitch = txtDesc.Width * sizeof(uint8_t) * 4;
    // InitData.SysMemSlicePitch = 0;

    m_device->CreateTexture2D(&txtDesc, &InitData, texture.GetAddressOf());
    m_device->CreateShaderResourceView(texture.Get(), nullptr,
                                       textureResourceView.GetAddressOf());
}

// dds파일을 읽어들이고 초기화한다.
void AppBase::CreateCubemapTexture(
    const wchar_t *filename,
    ComPtr<ID3D11ShaderResourceView> &textureResourceView) {

    ComPtr<ID3D11Texture2D> texture;

    // DDS파일에서 큐브맵 텍스처를 읽어서 GPU메모리에 올리고, 그걸 셰이더에서 쓸 수 있도록 SRV까지 생성해준다.
    auto hr = CreateDDSTextureFromFileEx(
        m_device.Get(), filename, 0, D3D11_USAGE_DEFAULT,
        D3D11_BIND_SHADER_RESOURCE, 0, D3D11_RESOURCE_MISC_TEXTURECUBE,
        DDS_LOADER_FLAGS(false), (ID3D11Resource **)texture.GetAddressOf(),
        textureResourceView.GetAddressOf(), nullptr);
    
    if (FAILED(hr)) {
        std::cout << "CreateDDSTextureFromFileEx() failed" << std::endl;
    }
}

} // namespace hlab