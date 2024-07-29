#include "RainWindow.h"

//
// Provides the entry point to the application.
//
int WINAPI WinMain(
	HINSTANCE hInstance,
	HINSTANCE /*hPrevInstance*/,
	LPSTR /*lpCmdLine*/,
	int /*nCmdShow*/
)
{
	// Ignore the return value because we want to continue running even in the
	// unlikely event that HeapSetInformation fails.
	HeapSetInformation(nullptr, HeapEnableTerminationOnCorruption, nullptr, 0);

	if (SUCCEEDED(CoInitialize(NULL)))
	{
		{
			RainWindow mainWindow;
			if (SUCCEEDED(mainWindow.Initialize(hInstance)))
			{
				mainWindow.RunMessageLoop();
			}
		}
		CoUninitialize();
	}

	return 0;
}
