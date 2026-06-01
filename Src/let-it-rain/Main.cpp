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
			// Single-threaded render loop driving one waitable swap chain per
			// monitor. Each monitor's frame-latency waitable signals at its own
			// vsync, so monitors with different refresh rates are paced
			// independently without one blocking Present stalling the others.
			bool running = true;
			while (running)
			{
				// Collect the waitable handles of monitors that are currently
				// renderable. Locked / device-lost windows are serviced below and
				// must NOT have their waitable consumed without a matching Present
				// (doing so can leave the handle un-signaled and stall the window).
				HANDLE waitHandles[MAXIMUM_WAIT_OBJECTS];
				DWORD waitCount = 0;
				for (DisplayWindow* rainWindow : rainWindows)
				{
					if (rainWindow->IsRenderable() && waitCount < MAXIMUM_WAIT_OBJECTS - 1)
					{
						waitHandles[waitCount++] = rainWindow->GetFrameLatencyWaitable();
					}
				}

				// Sleep until a monitor is ready for a new frame, a message
				// arrives, or the safety timeout elapses. The timeout covers the
				// all-idle case (e.g. session locked) and guarantees forward
				// progress if a waitable edge is ever missed.
				const DWORD waitResult = MsgWaitForMultipleObjectsEx(
					waitCount, waitHandles, 100, QS_ALLINPUT, MWMO_INPUTAVAILABLE);

				// Drain all pending window messages first.
				MSG msg;
				while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
				{
					if (msg.message == WM_QUIT)
					{
						running = false;
						break;
					}
					TranslateMessage(&msg);
					DispatchMessage(&msg);
				}
				if (!running) break;

				// MsgWaitForMultipleObjectsEx auto-resets only the single handle it
				// reports; remember it so that window still renders even though a
				// later poll of the same handle would now read as not-signaled.
				HANDLE signaledHandle = nullptr;
				if (waitResult >= WAIT_OBJECT_0 && waitResult < WAIT_OBJECT_0 + waitCount)
				{
					signaledHandle = waitHandles[waitResult - WAIT_OBJECT_0];
				}
				const bool timedOut = (waitResult == WAIT_TIMEOUT);

				for (DisplayWindow* rainWindow : rainWindows)
				{
					if (!rainWindow->IsRenderable())
					{
						// Device-lost windows recover inside Animate(); locked
						// windows early-return cheaply. Waitable left untouched.
						rainWindow->Animate();
						continue;
					}

					// Render this monitor only if its swap chain is ready. On the
					// safety timeout, render unconditionally to re-arm the waitable
					// and guarantee progress.
					const HANDLE h = rainWindow->GetFrameLatencyWaitable();
					if (timedOut || h == signaledHandle ||
						WaitForSingleObject(h, 0) == WAIT_OBJECT_0)
					{
						rainWindow->Animate();
					}
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
