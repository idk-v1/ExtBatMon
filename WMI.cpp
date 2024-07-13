#include "WMI.hpp"

WMI::~WMI()
{
	if (svc)
		svc->Release();
	if (loc)
		loc->Release();
	CoUninitialize();
}

bool WMI::init()
{
	if (FAILED(CoInitializeEx(0, COINIT_MULTITHREADED)))
	{
		printf("ERROR: Failed to init COM library\n");
		return false;
	}

	if (FAILED(CoInitializeSecurity(0, -1, 0, 0, 0, 3, 0, 0, 0)))
	{
		CoUninitialize();
		printf("ERROR: Failed to init security\n");
		return false;
	}

	if (FAILED(CoCreateInstance(CLSID_WbemLocator, 0, 1, IID_IWbemLocator, (void**)&loc)))
	{
		CoUninitialize();
		printf("ERROR: Failed to create locator\n");
		return false;
	}

	return true;
}

bool WMI::connect(const std::wstring& location)
{
	if (svc)
	{
		svc->Release();
		svc = nullptr;
	}

	if (FAILED(loc->ConnectServer(_bstr_t(location.data()), 0, 0, 0, 0, 0, 0, &svc)))
	{
		loc->Release();
		CoUninitialize();
		printf("ERROR: Failed to connect to WMI\n");
		return false;
	}

	if (FAILED(CoSetProxyBlanket(svc, 10, 0, 0, 3, 3, 0, 0)))
	{
		svc->Release();
		loc->Release();
		CoUninitialize();
		printf("ERROR: Failed to set proxy blanket\n");
		return false;
	}

	return true;
}

_variant_t WMI::query(const std::wstring& name, const std::wstring& from)
{
	IEnumWbemClassObject* pEnum = 0;
	HRESULT status = svc->ExecQuery(_bstr_t(L"WQL"),
		_bstr_t(L"SELECT ") + name.data() + _bstr_t(L" FROM ") + from.data(),
		48, 0, &pEnum);
	if (FAILED(status))
	{
		printf("ERROR: Query failed\n");
		return false;
	}

	IWbemClassObject* pclsObj = nullptr;
	ULONG ret = 0;
	_variant_t var;
	HRESULT hr = pEnum->Next(-1, 1, &pclsObj, &ret);
	hr = pclsObj->Get(name.data(), 0, var.GetAddress(), 0, 0);
	pclsObj->Release();

	pEnum->Release();

	return var;
}
