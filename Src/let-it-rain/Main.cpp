#include "DisplayWindow.h"
#include "Global.h"
#include <chrono>
#include <thread> // Add thread header for sleep_for

// Helper function to limit frame rate
void sleepFor(const std::chrono::high_resolution_clock::time_point& lastFrameTime, 
              const std::chrono::microseconds& targetFrameTime) {
    // Calculate time since last frame
    auto now = std::chrono::high_resolution_clock::now();
    auto frameTime = now - lastFrameTime;
    
    // If we still have time before next frame is due
    if (frameTime < targetFrameTime) {
        auto sleepTime = targetFrameTime - frameTime;
        std::this_thread::sleep_for(sleepTime);
    }
}

//
// Provides the entry point to the application.
//
int WINAPI WinMain(
	const HINSTANCE hInstance,
	HINSTANCE /*hPrevInstance*/,
	LPSTR /*lpCmdLine*/,
	int /*nCmdShow*/
)
{
	// Ignore the return value because we want to continue running even in the
	// unlikely event that HeapSetInformation fails.
	HeapSetInformation(nullptr, HeapEnableTerminationOnCorruption, nullptr, 0);

	SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);

	std::vector<MonitorData> monitorDataList;
	EnumDisplayMonitors(nullptr, nullptr, MonitorEnumProc, reinterpret_cast<LPARAM>(&monitorDataList));

	if (SUCCEEDED(CoInitialize(NULL)))
	{
		std::vector<DisplayWindow*> rainWindows;
		for (const auto& monitorData : monitorDataList)
		{
			DisplayWindow* rainWindow = new DisplayWindow();
			if (SUCCEEDED(rainWindow->Initialize(hInstance, monitorData)))
			{
				rainWindows.push_back(rainWindow);
			}
		}

		if (!rainWindows.empty())
		{
			MSG msg = {};
            
            // Target 60 FPS - approximately 16.6667 milliseconds per frame
            const std::chrono::microseconds targetFrameTime(16667);
            auto lastFrameTime = std::chrono::high_resolution_clock::now();
            
			while (msg.message != WM_QUIT)
			{
				if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
				{
					TranslateMessage(&msg);
					DispatchMessage(&msg);
				}
				else
				{
					for (DisplayWindow* rainWindow : rainWindows)
					{
						rainWindow->Animate();
					}
                    
                    // Properly limit the frame rate
                    sleepFor(lastFrameTime, targetFrameTime);
                    lastFrameTime = std::chrono::high_resolution_clock::now();
				}
			}
			for (const DisplayWindow* rainWindow : rainWindows)
			{
				delete rainWindow;
			}
		}
		CoUninitialize();
	}
	return 0;
}
