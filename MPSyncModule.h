#pragma once

#include "Modules/ModuleManager.h"

class FMPSyncModule : public FDefaultGameModuleImpl {
public:
	virtual void StartupModule() override;

	virtual bool IsGameModule() const override { return true; }
	void getHost(char* outStr);
	void getAuth(char* outStr);
};