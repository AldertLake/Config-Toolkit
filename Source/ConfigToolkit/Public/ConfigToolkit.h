// ----------------------------------------------------------------------------------
// Copyright (c) 2026 AldertLake. All Rights Reserved.
// GitHub:   https://github.com/AldertLake/
// Freelance:  https://www.upwork.com/freelancers/~01f46dab6bbf4fe99e?mp_source=share
// ----------------------------------------------------------------------------------

#pragma once

#include "Modules/ModuleManager.h"

DECLARE_LOG_CATEGORY_EXTERN(LogConfigToolkit, Log, All);

class FConfigToolkitModule : public IModuleInterface
{
public:

	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
};
