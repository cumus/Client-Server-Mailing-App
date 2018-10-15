#include "ModuleMainMenu.h"
#include "ModuleClient.h"
#include "ModuleServer.h"
#include "Application.h"
#include "Log.h"
#include "imgui/imgui.h"

bool ModuleMainMenu::update()
{
	ImGui::Begin("Main menu");

	if (ImGui::Button("Client"))
	{
		LOG("Client");
		setActive(false);
		App->modClient->setActive(true);
	}

	if (ImGui::Button("Server"))
	{
		LOG("Server");
		setActive(false);
		App->modServer->setActive(true);
	}

	ImGui::End();

	return true;
}
