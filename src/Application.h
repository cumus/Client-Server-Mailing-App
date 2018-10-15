#pragma once

#include <vector>

// Module declarations
class Module;
class ModuleWindow;
class ModuleMainMenu;
class ModuleClient;
class ModuleServer;
class ModuleLogView;

class Application
{
public:

	// Constructor and destructor

	Application();

	~Application();


	// Application methods

	bool wantsToExit() const { return wannaExit; }

	void exit() { wannaExit = true; }


	// Application lifetime methods

	bool start();

	bool update();

	bool cleanUp();


private:

	// Private lifetime methods

	bool doPreUpdate();
	
	bool doUpdate();
	
	bool doPostUpdate();


public:

	// Modules
	ModuleWindow *modWindow = nullptr;
	ModuleMainMenu *modMainMenu = nullptr;
	ModuleClient *modClient = nullptr;
	ModuleServer *modServer = nullptr;
	ModuleLogView *modLogView = nullptr;


private:

	// All modules
	std::vector<Module*> modules;

	// Exit flag
	bool wannaExit = false;
};

extern Application* App;
