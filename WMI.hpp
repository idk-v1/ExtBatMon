#include <string>
#include <comdef.h>
#include <WbemIdl.h>
#pragma comment (lib, "wbemuuid.lib")

class WMI
{
public:
	~WMI();

	bool init();

	bool connect(const std::wstring& location);

	_variant_t query(const std::wstring& name, const std::wstring& from);
private:
	IWbemLocator* loc = nullptr;
	IWbemServices* svc = nullptr;
};