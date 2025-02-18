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

bool CanRunTest(TestResultStatus status)
{
	return !TestManager::Instance().IsRunningTests();
}

void OpenTestFileToIssue(const TestObject& test)
{
	// if there's a test failure then goto it's line definition
	int lineNumber = test.LineNumber;
	if (const auto* result = TestManager::Instance().FetchResult(&test))
	{
		if (result->_lastFailure)
		{
			lineNumber = result->_lastFailure->linenumber();
		}
	}

	// TODO: we may need to find devenv
	std::string cmd = std::format("devenv.exe /edit {0} /command \"Edit.GoTo {1}\"", test.File, test.LineNumber);
	std::system(cmd.c_str());
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

constexpr const char* ToCString(TestConcurrency concurrency)
{
	switch (concurrency)
	{
		case TestConcurrency::Exclusive: return "Excl";
		case TestConcurrency::Privileged: return "Priv";
		case TestConcurrency::Any: return "Mult";
	}

	return "<unknown>";
}

void ImGuiPanel_TestManager::OnImGui()
{
	auto& testManager = TestManager::Instance();
	if (!testManager.IsRunningTests() && ImGui::Button("Test All"))
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

void DisplayTestDetails(const TestObject& test, TestResultStatus status)
{

	if (CanRunTest(status) && ImGui::Button("Run"))
	{
		TestManager::Instance().Run(test);
	}

	ImGui::SameLine();
	if (ImGui::Button("Goto"))
	{
		OpenTestFileToIssue(test);
	}

	ImGui::SameLine();
	ImGui::TextColored(ToColor(status), XEnumTraits<decltype(status)>::ToCString(status));
}

void ImGuiPanel_TestManager::OnImGui(const TestObject& test)
{
	auto id = ImGui::Scoped::Id(test.Id);
	auto status = TestManager::Instance().DetermineStatus(&test);

	if (test.Children.size())
	{
		auto cs = ImGui::Scoped::TreeNode(test.Name.c_str());
		ImGui::SameLine();
		DisplayTestDetails(test, status);

		if (cs)
		{
			auto indent = ImGui::Scoped::Indent(0.1f);

			// order the results based on depth of sub results.
			auto children = test.GetChildren();
			std::sort(children.begin(), children.end(), [](const auto* lhs, const auto* rhs)
			{
				return lhs->Children.size() > rhs->Children.size();
			});
			for (const auto* child: children)
				OnImGui(*child);
		}
	}
	else if (test.Definition)
	{
		ImGui::Text("%s (%s)", test.Name.c_str(), ToCString(test.Definition->Concurrency));
		ImGui::SameLine();
		DisplayTestDetails(test, status);

		const auto* result = TestManager::Instance().FetchResult(&test);
		if (result)
		{
			ImGui::SameLine();
			ImGui::Text("Time Taken %d (ns)", result->TimeTaken());
		}

		if (status == TestResultStatus::Failed)
		{
			const auto& failure = result->_lastFailure.value();
			// Print the error message
			ImGui::TextColored(TestStatusColors::Failed, failure.FormattedString().c_str());
		}
	}
}
