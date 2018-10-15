#include "Application.h"
#include "ModuleWindow.h"
#include "ModuleMainMenu.h"
#include "ModuleClient.h"
#include "ModuleServer.h"
#include "ModuleLogView.h"
#include "SocketUtils.h"

#define ADD_MODULE(ModuleClass, moduleAttribute) \
	moduleAttribute = new ModuleClass(); \
	modules.push_back(moduleAttribute);

static Application *g_Instance = nullptr;

Application::Application()
{
	// Create modules
	ADD_MODULE(ModuleWindow, modWindow);
	ADD_MODULE(ModuleMainMenu, modMainMenu);
	ADD_MODULE(ModuleClient, modClient);
	ADD_MODULE(ModuleServer, modServer);
	ADD_MODULE(ModuleLogView, modLogView);

	// Set active modules
	modWindow->setActive(true);
	modMainMenu->setActive(true);
	modLogView->setActive(true);
}


Application::~Application()
{
	// Destroy modules
	for (auto module : modules) {
		delete module;
	}
}


bool Application::start()
{
	initializeSocketsLibrary();

	for (auto module : modules) {
		if (module->start() == false) {
			return false;
		}
	}
	return true;
}

bool Application::update()
{
	if (doPreUpdate() == false) return false;
	
	if (doUpdate() == false) return false;

	if (doPostUpdate() == false) return false;

	return true;
}

bool Application::cleanUp()
{
	for (auto module : modules) {
		if (module->cleanUp() == false) {
			return false;
		}
	}

	cleanupSocketsLibrary();

	return true;
}

bool Application::doPreUpdate()
{
	for (auto module : modules)
	{
		if (module->isActive() == false) continue;
		
		if (module->preUpdate() == false) return false;
	}
	return true;
}

bool Application::doUpdate()
{
	for (auto module : modules)
	{
		if (module->isActive() == false) continue;

		if (module->update() == false) return false;
	}
	return true;
}

bool Application::doPostUpdate()
{
	for (auto module : modules)
	{
		if (module->isActive() == false) continue;

		if (module->postUpdate() == false) return false;
	}
	return true;
}
