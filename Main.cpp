#include <comdef.h>
#include <WbemIdl.h>
#pragma comment (lib, "wbemuuid.lib")

static IWbemServices* setupWMI()
{
	IWbemLocator* loc = NULL;
	IWbemServices* svc = NULL;

	HRESULT hr = 0;

	hr = CoInitializeEx(0, COINIT_MULTITHREADED);
	hr = CoInitializeSecurity(0, -1, 0, 0, 0, 3, 0, 0, 0);

	hr = CoCreateInstance(CLSID_WbemLocator, 0, 1, IID_IWbemLocator, (void**)&loc);


	loc->ConnectServer((wchar_t*)L"ROOT\\WMI", 0, 0, 0, 0, 0, 0, &svc);

	hr = CoSetProxyBlanket(svc, 10, 0, 0, 3, 3, 0, 0);

	loc->Release();

	return svc;
}


// https://stackoverflow.com/a/7785296
static uint32_t getBatteryCapacity(IWbemServices* svc)
{
	IEnumWbemClassObject* pEnum = 0;

	HRESULT status = svc->ExecQuery((wchar_t*)L"WQL",
		(wchar_t*)L"SELECT FullChargedCapacity FROM BatteryFullChargedCapacity",
		48, 0, &pEnum);


	IWbemClassObject* pclsObj = NULL;
	ULONG ret = 0;
	_variant_t var;
	HRESULT hr = pEnum->Next(-1, 1, &pclsObj, &ret);
	if (pclsObj)
	{
		hr = pclsObj->Get(L"FullChargedCapacity", 0, var.GetAddress(), 0, 0);
		pclsObj->Release();

		pEnum->Release();

		return var.ulVal;
	}
	return 0;
}

static uint32_t getBatteryRemaining(IWbemServices* svc)
{
	IEnumWbemClassObject* pEnum = 0;

	HRESULT status = svc->ExecQuery((wchar_t*)L"WQL",
		(wchar_t*)L"SELECT RemainingCapacity FROM BatteryStatus",
		48, 0, &pEnum);


	IWbemClassObject* pclsObj = NULL;
	ULONG ret = 0;
	_variant_t var;
	HRESULT hr = pEnum->Next(-1, 1, &pclsObj, &ret);
	if (pclsObj)
	{
		hr = pclsObj->Get(L"RemainingCapacity", 0, var.GetAddress(), 0, 0);
		pclsObj->Release();

		pEnum->Release();

		return var.ulVal;
	}
	return 0;
}


static void enableVT()
{
	HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
	uint32_t consoleMode = 0;
	GetConsoleMode(hOut, (DWORD*)&consoleMode);
	consoleMode |= ENABLE_PROCESSED_OUTPUT | ENABLE_VIRTUAL_TERMINAL_PROCESSING;
	SetConsoleMode(hOut, consoleMode);
}

#define CURSOR(x, y) "\x1B["#y";"#x"H"


static uint64_t timeDiff(uint64_t* last)
{
	uint64_t time = 0;
	QueryPerformanceCounter((LARGE_INTEGER*)&time);

	uint64_t diff = time - *last;
	*last = time;

	return diff;
}

int main()
{
	uint32_t lastCharge = 0, batteryCapacity = 0, batteryCharge = 0;
	uint64_t updTime = 0;
	uint64_t changeTime = 0;
	uint64_t deltaTime = 0;

	IWbemServices* wmi = setupWMI();

		
	changeTime = timeDiff(&updTime);


	batteryCapacity = getBatteryCapacity(wmi);
	batteryCharge = getBatteryRemaining(wmi);
	lastCharge = batteryCharge;

	while (true)
	{
		deltaTime += timeDiff(&updTime);

		if (deltaTime > 10'000'000)
		{
			deltaTime = 0;

			batteryCapacity = getBatteryCapacity(wmi);
			batteryCharge = getBatteryRemaining(wmi);

			printf(CURSOR(1, 1) "Battery Charge: %d/%d (%.2f%%)               " CURSOR(1, 3), 
				batteryCharge, 
				batteryCapacity, 
				batteryCharge * 100.f / batteryCapacity);

			if (lastCharge != batteryCharge)
			{
				uint64_t changeDiff = timeDiff(&changeTime);
				printf(CURSOR(1, 2) "Changed (%+.2f%% %.1fsec)" CURSOR(1, 3), 
					((batteryCharge * 100.f - lastCharge * 100.f) / batteryCapacity),
					(changeDiff / 1'000'000) / 10.f);
			}
			else
				printf(CURSOR(1, 2) "                                      " CURSOR(1, 3));

			lastCharge = batteryCharge;
		}

		if (GetKeyState(VK_ESCAPE) & 0x8000)
			break;

		Sleep(10);
	}

	wmi->Release();
	
	return 0;
}