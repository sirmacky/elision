#include "ImGuiService.h"
#include "foundation/imgui.h"

#include "ImGuiPanels/ImGuiPanel.h"
#include "ImGuiPanels/ImGuiPanel_TestManager.h"

ImplementXEnum(TestEnum,
    XValue(value1),
    XValue(valuefgsfvsdfgsdgfsdfg2),
    XValue(value3)
);

// TODO: Creat a test panel

class TestPanel : public ImGuiPanel
{
    int counter = 0;
    float f = 0.0f;
    std::string str;
    TestEnum var = TestEnum::value1;

    virtual void OnImGui() override
    {
        ImGui::Text("This is some useful text.");               // Display some text (you can use a format strings too)

        ImGui::SliderFloat("float", &f, 0.0f, 1.0f);            // Edit 1 float using a slider from 0.0f to 1.0f

        if (ImGui::Button("Button"))                            // Buttons return true when clicked (most widgets return true when edited/activated)
            counter++;
        ImGui::SameLine();
        ImGui::Text("counter = %d", counter);

        ImGui::InputText("Input some text", &str);
        ImGui::Text("Entered text: %s", str.c_str());
        ImGui::Combo(var);
    }
};

ImGuiService::ImGuiService()
{
    // TODO: Move these outta here
    RegisterPanel<TestPanel>("TestPanel");
    RegisterPanel<ImGuiPanel_TestManager>("TestManager");
}

void ImGuiService::OnImGui()
{
    // TODO: 
    bool open = true;
    for (auto& ptr : Panels)
    {
        if (ImGui::Begin(ptr->Name().c_str(), &open))
        {
            ptr->OnImGui();
        }
        ImGui::End();
    }
}