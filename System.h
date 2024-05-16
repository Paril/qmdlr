#pragma once

#include <optional>
#include <memory>
#include <algorithm>

// manages the OS stuff, main loop
// and provides access to secondary classes
class System
{
	std::unique_ptr<struct SystemPrivate> priv;
	int _wantRedraw = 5;

public:
	System();
	std::optional<const char *> Init();
	bool Run();
	void Shutdown();
	
	void WantsRedraw()
	{
		_wantRedraw = std::max(1, _wantRedraw);
	}
};

System &sys();