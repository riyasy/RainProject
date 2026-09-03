// header.h : include file for standard system include files,
// or project specific include files
//

#pragma once

#ifndef NTDDI_VERSION
#define NTDDI_VERSION 0x0A000000 // Windows 10
#endif

// Minimum supported OS is Windows 10: required for ID2D1DeviceContext3 sprite
// batches, frame-latency waitable swap chains, and DirectComposition.
#ifndef WINVER
#define WINVER 0x0A00
#endif

#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0A00
#endif

#ifndef UNICODE
#define UNICODE
#endif


#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN // Exclude rarely-used stuff from Windows headers
#endif

// Windows Header Files
#include <windows.h>
// C RunTime Header Files
#include <cstdlib>
#include <malloc.h>
#include <memory.h>
#include <tchar.h>

#pragma comment(lib, "dxgi")
#pragma comment(lib, "d3d11")
#pragma comment(lib, "d2d1")
#pragma comment(lib, "dcomp")
// The Common Controls 6 dependency moved to app.manifest, which also carries
// the DPI and supportedOS declarations. Declaring it in both places would put
// two copies of the same assemblyIdentity into the merged manifest.
#pragma comment(lib, "comctl32.lib")

#define WM_TRAYICON (WM_USER + 1)

// Uncomment to enable on-screen FPS counter (top-right corner). No code runs when commented out.
// #define SHOW_FPS
