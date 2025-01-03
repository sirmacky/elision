#pragma once

#include <string>
#include <memory>
#include <vector>

#include "ImGuiPanels/ImGuiPanel.h"

class ImGuiService
{
	std::vector<std::unique_ptr<ImGuiPanel>> Panels;
	struct PanelContext
	{
		// data pertaining to the panel
	};

public:

	ImGuiService();

	template<typename T>
	void RegisterPanel(const std::string& name)
	{
		auto panel = std::make_unique<T>();
		panel->_name = name;
		Panels.emplace_back(std::move(panel));
	}

	void OnImGui();
};