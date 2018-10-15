#pragma once

#include "Module.h"

class ModuleWindow : public Module
{
public:

	// Virtual functions

	bool start() override;

	bool preUpdate() override;

	bool postUpdate() override;

	bool cleanUp() override;
};