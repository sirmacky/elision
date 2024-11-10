#include "ImGuiPanel_TestManager.h"
#include "Foundation/imgui.h"
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
	
	for (const auto& category : TestManager::Instance()._categories)
		OnImGui(category);
}

void ImGuiPanel_TestManager::OnImGui(const TestCategory& category)
{
	if (auto cs = ImGui::Scoped::TreeNode(category.Name.c_str()))
	{
		ImGui::SameLine();
		if (ImGui::Button("Run"))
		{
			TestManager::Instance().Run(category);
		}
		OnImGuiDetails(category);
	}
}

void ImGuiPanel_TestManager::OnImGuiDetails(const TestCategory& category)
{
	auto indent = ImGui::Scoped::Indent(0.1f);

	for (const auto& subCategory : category.SubCategories)
		if (!subCategory->IsEmpty())
			OnImGui(*subCategory.get());

	for (const auto& test : category.Tests)
		OnImGui(test);
}


void ImGuiPanel_TestManager::OnImGui(const TestDefinition& definition)
{
	auto scope = ImGui::Scoped::TreeNode(definition._name.c_str());
	auto idScope = ImGui::Scoped::Id(definition._name);

	const auto* result = TestManager::Instance().FetchResult(&definition);
	auto status = result->Status();

	ImGui::SameLine();
	ImGui::TextColored(ToColor(status), XEnumTraits<decltype(status)>::ToCString(status));

	ImGui::SameLine();
	if (ImGui::Button("Run"))
	{
		TestManager::Instance().Run(definition);
	}

	if (scope)
	{
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
