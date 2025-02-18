#include "ImGuiPanel_TestManager.h"
#include "Foundation/imgui.h"
#include "TestFramework/TestManager.h"
#include "TestFramework/TestRunner.h"
#include "TestFramework/TestObject.h"

#include <algorithm>

using namespace lsn::test_framework;

namespace TestStatusColors
{
	constexpr ImVec4 Passed       { 0.20f, 0.84f, 0.20f, 1.0f };
	constexpr ImVec4 NotRun       { 0.30f, 0.40f, 0.20f, 1.0f };
	constexpr ImVec4 Failed       { 0.84f, 0.20f, 0.20f, 1.0f };
	constexpr ImVec4 WaitingToRun { 0.30f, 0.40f, 0.40f, 1.0f };
	constexpr ImVec4 Running      { 0.20f, 0.64f, 0.20f, 1.0f };
}


constexpr ImVec4 ToColor(TestResultStatus status)
{
	switch (status)
	{
		case TestResultStatus::Passed: return TestStatusColors::Passed;
		case TestResultStatus::NotRun: return TestStatusColors::NotRun;
		case TestResultStatus::Failed: return TestStatusColors::Failed;
		case TestResultStatus::WaitingToRun: return TestStatusColors::WaitingToRun;
		case TestResultStatus::Running: return TestStatusColors::Running;
	}
		
	return TestStatusColors::NotRun;
}

void ImGuiPanel_TestManager::OnImGui()
{
	auto& testManager = TestManager::Instance();
	if (ImGui::Button("Test All"))
	{
		testManager.RunAll();
	}

	if (testManager.IsRunningTests() && ImGui::Button("Cancel"))
	{
		testManager.Cancel();
	}

	for (const auto& category : testManager._categories)
		OnImGui(category);
}

void ImGuiPanel_TestManager::OnImGui(const TestObject& category)
{

	//auto scope = ImGui::Scoped::TreeNode(category.Name.c_str());
	///auto idScope = ImGui::Scoped::Id(category.Name);

	auto cs = ImGui::Scoped::TreeNode(category.Name.c_str());
	ImGui::SameLine();

	if (ImGui::Button("Run"))
	{
		TestManager::Instance().Run(category);
	}
	auto status = TestManager::Instance().DetermineStatus(&category);

	ImGui::SameLine();
	ImGui::TextColored(ToColor(status), XEnumTraits<decltype(status)>::ToCString(status));
	
	if (cs)
	{
		auto indent = ImGui::Scoped::Indent(0.1f);

		for (const auto& subCategory : category.Children)
			OnImGui(*subCategory.get());

		if (category.Definition)
		{
			const auto* result = TestManager::Instance().FetchResult(category.Definition.get());
			if (status == TestResultStatus::Failed)
			{
				const auto& failure = result->_lastFailure.value();
				// Print the error message
				ImGui::TextColored(TestStatusColors::Failed, failure.FormattedString().c_str());

				ImGui::SameLine();
				if (ImGui::Button("goto"))
				{
					// need the devenv path

					// if has a devenv

					std::string cmd = std::format("devenv.exe /edit {0} /command \"Edit.GoTo {1}\"", failure.filename(), failure.linenumber());
					std::system(cmd.c_str());
				}
			}

			ImGui::LabelText("Last Run:", "%F", result->_timeStarted);
			ImGui::LabelText("Time Taken (ns):", "%d", result->TimeTaken());
		}
	}
}