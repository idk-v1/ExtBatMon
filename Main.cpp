#include "WMI.hpp"
#include <powersetting.h>

int main()
{
	SYSTEM_POWER_STATUS powerStatus;

	uint64_t nsCount = 0;

	WMI wmi;
	if (wmi.init() && wmi.connect(L"ROOT\\WMI"))
	{
		ULONG lastCharge = 0, batteryCapacity, batteryCharge;
		LARGE_INTEGER perfStart, perfEnd;
		QueryPerformanceCounter(&perfStart);

		while (!(GetKeyState(VK_ESCAPE) < 0))
		{
			QueryPerformanceCounter(&perfEnd);
			nsCount += perfEnd.QuadPart - perfStart.QuadPart;
			QueryPerformanceCounter(&perfStart);
			if (nsCount > 10000000)
			{
				nsCount = 0;

				batteryCapacity = wmi.query(
					L"FullChargedCapacity",
					L"BatteryFullChargedCapacity").ulVal;
				batteryCharge = wmi.query(
					L"RemainingCapacity",
					L"BatteryStatus").ulVal;

				// "Temporary" output to console
				system("cls");
				printf("Battery Charge: %d/%d (%.2f%%)\n", batteryCharge, 
					batteryCapacity, batteryCharge * 100.f / batteryCapacity);
				if (lastCharge != batteryCharge)
					printf("Changed\n");
				
				lastCharge = batteryCharge;

				// Battery hardware only updates values every few seconds
				// WMI updates values every 2ish minutes from my testing

				// Changing a battery setting forces WMI to update the battery values
				// I haven't found any other way of doing this,
				// and nobody has reverse engineered it yet

				// Get if the device is using a battery or not
				// Only spam powercfg with either ac or dc to be less wastefull lmao
				GetSystemPowerStatus(&powerStatus);
				if (powerStatus.ACLineStatus)
					system("powercfg /setacvalueindex SCHEME_CURRENT SUB_ENERGYSAVER ESBATTTHRESHOLD 100");
				else
					system("powercfg /setdcvalueindex SCHEME_CURRENT SUB_ENERGYSAVER ESBATTTHRESHOLD 100");
				system("powercfg /setactive scheme_current");

				if (powerStatus.ACLineStatus)
					system("powercfg /setacvalueindex SCHEME_CURRENT SUB_ENERGYSAVER ESBATTTHRESHOLD 0");
				else
					system("powercfg /setdcvalueindex SCHEME_CURRENT SUB_ENERGYSAVER ESBATTTHRESHOLD 0");
				system("powercfg /setactive scheme_current");
			}
		}

		return 0;
	}
	else
	{
		printf("Failed to connect to WMI\n");
		return 1;
	}

}