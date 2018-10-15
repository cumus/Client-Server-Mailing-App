#pragma once

class Module
{
public:

	// Constructor and destructor

	Module() { }

	virtual ~Module() { }


	// Common functions

	bool isActive() const { return active; }

	void setActive(bool a) { active = a; }


	// Virtual functions

	virtual bool start() { return true; }

	virtual bool preUpdate() { return true; }

	virtual bool update() { return true; }

	virtual bool postUpdate() { return true; }

	virtual bool cleanUp() { return true;  }

private:

	bool active = false;
};

