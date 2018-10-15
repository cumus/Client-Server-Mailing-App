#include "ModuleLogView.h"
#include "Log.h"
#include "imgui/imgui.h"

bool ModuleLogView::update()
{
	ImGui::Begin("Log View");

	for (int i = 0; i < logLineCount(); ++i)
	{
		const char *line = logLineAt(i);

		ImGui::TextWrapped("%s", line);
	}
	
	ImGui::End();

	return true;
}
