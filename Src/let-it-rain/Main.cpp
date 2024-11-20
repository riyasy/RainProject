#include "DisplayWindow.h"
#include "Global.h"


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
					Sleep(10);
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
