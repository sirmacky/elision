#pragma once

#include "ImGuiPanel.h"

struct TestObject;
struct TestDefinition;

class ImGuiPanel_TestManager : public ImGuiPanel
{
public:
	virtual void OnImGui() override;
protected:
	void OnImGui(const TestObject& category);
	void OnImGui(const TestDefinition& instance);
};