#pragma once

#include "ImGuiPanel.h"

struct TestCategory;
struct TestDefinition;


class ImGuiPanel_TestManager : public ImGuiPanel
{
public:
	virtual void OnImGui() override;
protected:
	void OnImGui(const TestCategory& category);
	void OnImGuiDetails(const TestCategory& category);
	void OnImGui(const TestDefinition& instance);
};

