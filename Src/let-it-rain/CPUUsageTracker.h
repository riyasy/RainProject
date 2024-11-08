#pragma once

#include <windows.h>
#include <pdh.h>
#include <iostream>

#pragma comment(lib, "pdh.lib")

class CPUUsageTracker
{
public:
	static CPUUsageTracker& getInstance()
	{
		static CPUUsageTracker instance;
		return instance;
	}

	double GetCPUUsage()
	{
		if (!cpuQuery)
		{
			return -1.0; // Return -1.0 on failure
		}

		LARGE_INTEGER currentTime;
		QueryPerformanceCounter(&currentTime);

		if (isFirstCall)
		{
			PdhCollectQueryData(cpuQuery);
			isFirstCall = false;
			lastQueryTime = currentTime;
			return 0.0; // Return 0.0 on the first call
		}

		// Time elapsed since last call
		const double elapsedTime = static_cast<double>(currentTime.QuadPart - lastQueryTime.QuadPart) / frequency.QuadPart;
		lastQueryTime = currentTime;

		// Collect data again for accurate results
		if (PdhCollectQueryData(cpuQuery) != ERROR_SUCCESS)
		{
			std::cerr << "PdhCollectQueryData failed" << '\n';
			return -1.0; // Return -1.0 on failure
		}

		PDH_FMT_COUNTERVALUE counterVal;
		if (PdhGetFormattedCounterValue(cpuTotal, PDH_FMT_DOUBLE, nullptr, &counterVal) == ERROR_SUCCESS)
		{
			return counterVal.doubleValue / elapsedTime; // Adjust usage based on elapsed time
		}
		std::cerr << "PdhGetFormattedCounterValue failed" << '\n';
		return -1.0; // Return -1.0 on failure
	}

private:
	PDH_HQUERY cpuQuery;
	PDH_HCOUNTER cpuTotal;
	bool isFirstCall;
	LARGE_INTEGER lastQueryTime, frequency;

	CPUUsageTracker() : cpuQuery(nullptr), cpuTotal(nullptr), isFirstCall(true)
	{
		QueryPerformanceFrequency(&frequency);
		if (PdhOpenQuery(nullptr, NULL, &cpuQuery) != ERROR_SUCCESS)
		{
			std::cerr << "PdhOpenQuery failed" << '\n';
		}
		else if (PdhAddCounter(cpuQuery, TEXT("\\Processor Information(_Total)\\% Processor Utility"), NULL, &cpuTotal)
			!= ERROR_SUCCESS)
		{
			std::cerr << "PdhAddCounter failed" << '\n';
			PdhCloseQuery(cpuQuery);
			cpuQuery = nullptr;
		}
	}

	~CPUUsageTracker()
	{
		if (cpuQuery)
		{
			PdhCloseQuery(cpuQuery);
		}
	}

	// Delete copy constructor and assignment operator to prevent copying
	CPUUsageTracker(const CPUUsageTracker&) = delete;
	CPUUsageTracker& operator=(const CPUUsageTracker&) = delete;
};

//int main()
//{
//	CPUUsageTracker& tracker = CPUUsageTracker::getInstance();
//
//	LARGE_INTEGER frequency, start, end;
//	QueryPerformanceFrequency(&frequency);
//
//	while (true)
//	{
//		QueryPerformanceCounter(&start);
//
//		double cpuUsage = tracker.GetCPUUsage();
//
//		QueryPerformanceCounter(&end);
//		double elapsedTime = static_cast<double>(end.QuadPart - start.QuadPart) / frequency.QuadPart * 1000.0;
//
//		if (cpuUsage >= 0.0)
//		{
//			std::cout << "CPU Usage: " << cpuUsage << "%" << '\n';
//		}
//		else
//		{
//			std::cerr << "Failed to retrieve CPU usage" << '\n';
//		}
//
//		std::cout << "Time taken for GetCPUUsage: " << elapsedTime << " ms" << '\n';
//
//		Sleep(1000); // Simulate calling the function every 1 second
//	}
//
//	return 0;
//}
