#include <windows.h>
#include <d2d1.h>
#include <cstdlib>
#include <ctime>
#include <vector>
#include <algorithm>
#include "RainDrop.h"

#pragma comment(lib, "d2d1")

// Global Declarations
ID2D1Factory* pFactory = NULL;
ID2D1HwndRenderTarget* pRenderTarget = NULL;
ID2D1SolidColorBrush* pBrush = NULL;

// Constants
float windowWidth = 100;
float windowHeight = 100;



// Function Prototypes
void CreateResources(HWND hwnd);
void DiscardResources();
void OnPaint(HWND hwnd);
void UpdateRainDrops();
void DrawRainDrops();
void CheckAndGenerateRainDrops();

// Global Variables
std::vector<RainDrop*> rainDrops;

LRESULT CALLBACK WndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) {
    switch (message) {
    case WM_PAINT:
        OnPaint(hwnd);
        return 0;

    case WM_SIZE:
        if (pRenderTarget) {
            RECT rc;
            GetClientRect(hwnd, &rc);
            D2D1_SIZE_U size = D2D1::SizeU(rc.right, rc.bottom);
            pRenderTarget->Resize(size);
        }
        return 0;

    case WM_DESTROY:
        DiscardResources();
        for (auto drop : rainDrops) {
            delete drop;
        }
        PostQuitMessage(0);
        return 0;

    default:
        return DefWindowProc(hwnd, message, wParam, lParam);
    }
}

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE, PWSTR, int nCmdShow) {
    srand(static_cast<unsigned>(time(0)));

    // Register the window class.
    const wchar_t CLASS_NAME[] = L"Bouncing Ellipse Window Class";

    WNDCLASS wc = {};
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = CLASS_NAME;

    RegisterClass(&wc);

    DWORD overlay_style = WS_EX_TRANSPARENT | WS_EX_LAYERED | WS_EX_TOPMOST; // Using WS_EX_LAYERED loses the GPU-friendliness
    DWORD normal_style = WS_EX_NOREDIRECTIONBITMAP;

    // Create the window.
    HWND hwnd = CreateWindowEx(
        overlay_style,
        CLASS_NAME,
        L"Direct2D Bouncing Ellipse with Gravity",
        WS_POPUP | WS_VISIBLE,
        CW_USEDEFAULT, CW_USEDEFAULT, 600, 400,
        NULL,
        NULL,
        hInstance,
        NULL
    );

    if (hwnd == NULL) {
        return 0;
    }

    //SetWindowLong(hwnd, GWL_EXSTYLE, GetWindowLong(hwnd, GWL_EXSTYLE) | WS_EX_LAYERED );
    SetLayeredWindowAttributes(hwnd, RGB(255, 255, 255), 0, LWA_COLORKEY);
    ShowWindow(hwnd, SW_MAXIMIZE);

    // Initialize Direct2D
    HRESULT hr = D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, &pFactory);
    if (FAILED(hr)) {
        return -1;
    }

    RECT rc;
    GetClientRect(hwnd, &rc);
    windowWidth = static_cast<float>(rc.right - rc.left);
    windowHeight = static_cast<float>(rc.bottom - rc.top);

    // Run the message loop.
    MSG msg = {};
    DWORD lastTime = GetTickCount();
    while (true) {
        while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
            if (msg.message == WM_QUIT) {
                return 0;
            }
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }

        // Update and render
        DWORD currentTime = GetTickCount();
        if (currentTime - lastTime >= 16) { // ~60 FPS
            UpdateRainDrops();
            InvalidateRect(hwnd, NULL, FALSE);
            lastTime = currentTime;
        }

        // Process messages
        Sleep(10); // Reduce CPU usage
    }
}

void CreateResources(HWND hwnd) {
    if (!pRenderTarget) {
        RECT rc;
        GetClientRect(hwnd, &rc);

        D2D1_SIZE_U size = D2D1::SizeU(rc.right, rc.bottom);

        // Create a Direct2D render target.
        pFactory->CreateHwndRenderTarget(
            D2D1::RenderTargetProperties(),
            D2D1::HwndRenderTargetProperties(hwnd, size),
            &pRenderTarget
        );

        // Create a brush.
        pRenderTarget->CreateSolidColorBrush(
            D2D1::ColorF(D2D1::ColorF::LightGray),
            &pBrush
        );
    }
}

void DiscardResources() {
    if (pBrush) pBrush->Release();
    if (pRenderTarget) pRenderTarget->Release();
}

void OnPaint(HWND hwnd) {
    CreateResources(hwnd);

    PAINTSTRUCT ps;
    BeginPaint(hwnd, &ps);

    pRenderTarget->BeginDraw();

    pRenderTarget->Clear(D2D1::ColorF(D2D1::ColorF::White));

    // Draw the bouncing raindrop
    DrawRainDrops();


    HRESULT hr = pRenderTarget->EndDraw();

    if (FAILED(hr) || hr == D2DERR_RECREATE_TARGET) {
        DiscardResources();
    }

    EndPaint(hwnd, &ps);
}

void UpdateRainDrops() {
    for (auto drop : rainDrops) {
        drop->MoveToNewPosition();
    }
}

void DrawRainDrops() {
    CheckAndGenerateRainDrops();
    for (auto drop : rainDrops) {
        drop->Draw(pRenderTarget, pBrush);
    }
}

void CheckAndGenerateRainDrops() {
    // Move each raindrop to the next point
    for (auto drop : rainDrops) {
        drop->MoveToNewPosition();
    }

    // Remove all raindrops that have expired
    std::vector<RainDrop*>::iterator it;

    for (it = rainDrops.begin(); it != rainDrops.end(); ) {
        if ((*it)->ShouldBeErasedAndDeleted()) {
            delete* it;
            it = rainDrops.erase(it);
        }
        else {
            ++it;
        }
    }

    // Calculate the number of raindrops to generate
    int countOfFallingDrops = 0;
    for (auto drop : rainDrops) {
        if (!drop->DidDropLand())
        {
            countOfFallingDrops++;
        }
    }
    int noOfDropsToGenerate = 10 - countOfFallingDrops;

    // Generate new raindrops
    for (int i = 0; i < noOfDropsToGenerate; ++i) {
        RainDrop* k = new RainDrop(windowWidth, windowHeight, RainDropType::MainDrop);
        rainDrops.push_back(k);
    }
}
