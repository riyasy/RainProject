#pragma once

// Callback function to be called for each display
inline BOOL CALLBACK MonitorEnumProc(const HMONITOR hMonitor, HDC hdcMonitor, LPRECT lprcMonitor, const LPARAM lParam)
{
	std::vector<MonitorData>* monitorDataList = reinterpret_cast<std::vector<MonitorData>*>(lParam);
	MONITORINFOEX monitorInfo;
	monitorInfo.cbSize = sizeof(monitorInfo);
	if (GetMonitorInfo(hMonitor, &monitorInfo))
	{
		MonitorData monitorData;
		monitorData.MonitorRect = monitorInfo.rcMonitor;
		monitorData.Name = monitorInfo.szDevice;
		monitorData.IsPrimaryDisplay = (monitorInfo.dwFlags & MONITORINFOF_PRIMARY) != 0;
		
		monitorDataList->push_back(monitorData);
	}
	return TRUE;
}
