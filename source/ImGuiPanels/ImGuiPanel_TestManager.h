#pragma once

#include "ImGuiPanel.h"

namespace lsn::test_framework
{
	struct TestObject;
	struct TestDefinition;
}

class ImGuiPanel_TestManager : public ImGuiPanel
{
public:
	virtual void OnImGui() override;
protected:
	void OnImGui(const lsn::test_framework::TestObject& category);
	void OnImGui(const lsn::test_framework::TestDefinition& instance);
};