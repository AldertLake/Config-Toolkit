#pragma once

#include "Modules/ModuleManager.h"

class FConfigToolkitModule : public IModuleInterface
{
public:

	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
};
