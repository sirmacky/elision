#include "ImGuiPanel_TestManager.h"
#include "Foundation/imgui.h"
#include "TestFramework/TestManager.h"
#include "TestFramework/TestRunner.h"

#include <algorithm>

constexpr ImVec4 TestPassedColor{ 0.2f, 0.84f, 0.2f, 1.0f };
constexpr ImVec4 TestFailedColor{ 0.84f, 0.2f, 0.2f, 1.0f };
constexpr ImVec4 TestNotRunColor{ 0.3f, 0.4f, 0.2f, 1.0f };

constexpr ImVec4 ToColor(TestResultStatus status)
{
	if (status == TestResultStatus::Passed)
		return TestPassedColor;
	if (status == TestResultStatus::Failed)
		return TestFailedColor;

	return TestNotRunColor;
}

void ImGuiPanel_TestManager::OnImGui()
{
	if (ImGui::Button("Test All"))
	{
		TestManager::Instance().RunAll();
	}

	for (const auto& category : TestManager::Instance()._categories)
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
				ImGui::TextColored(TestFailedColor, failure.FormattedString().c_str());

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