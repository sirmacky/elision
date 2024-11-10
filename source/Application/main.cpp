#include <iostream>
#include <format>
#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include "foundation/imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

#include "TestFramework/TestRunner.h"

#include "ImGuiPanels/ImGuiPanel_TestManager.h"

ImplementXEnum(TestEnum,
    XValue(value1),
    XValue(valuefgsfvsdfgsdgfsdfg2),
    XValue(value3)
);

// Systems to build
// bootstrapper
// Logger
// Scopes
// Services
// Events / Multicast Delegates
// Imgui service
// application layer
// loading mechanisms
// json ?

int main()
{
    /* Initialize the library */
    if (!glfwInit())
        return -1;
    
    // GL 3.0 + GLSL 130
    const char* glsl_version = "#version 130";
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
    //glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);  // 3.2+ only
    //glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);            // 3.0+ only

    /* Create a windowed mode window and its OpenGL context */
    GLFWmonitor* monitor = glfwGetPrimaryMonitor();

    const GLFWvidmode* mode = glfwGetVideoMode(monitor);

    glfwWindowHint(GLFW_RED_BITS, mode->redBits);
    glfwWindowHint(GLFW_GREEN_BITS, mode->greenBits);
    glfwWindowHint(GLFW_BLUE_BITS, mode->blueBits);
    glfwWindowHint(GLFW_REFRESH_RATE, mode->refreshRate);
    glfwWindowHint(GLFW_MAXIMIZED, GLFW_TRUE);

    GLFWwindow* window = glfwCreateWindow(mode->width, mode->height, "My Title", NULL, NULL);

    if (!window)
    {
        glfwTerminate();
        return -1;
    }

    /* Make the window's context current */
    glfwMakeContextCurrent(window);

    if (auto result = glewInit(); result != GLEW_OK)
    {
        std::cout << "glew init failed: " << result << std::endl;
        return -1;
    }

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;         // Enable Docking
    io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;       // Enable Multi-Viewport / Platform Windows

    // Setup Dear ImGui style
    ImGui::StyleColorsDark();

    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init(glsl_version);
    
    //ImGui::StyleColorsLight();
    int counter = 0;
    float f = 0.0f;

    auto var = TestEnum::value1;
    std::string str;

    bool open = true;

    /* Loop until the user closes the window */
    while (!glfwWindowShouldClose(window))
    {
        /* Poll for and process events */
        glfwPollEvents();

        // TODO Update application here
        

        /* Render here */
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();
        {
            // Imgui render commands
        
            // Create a window called "Hello, world!" and append into it.
            if (ImGui::Begin("Hello, world!"))
            {
                ImGui::Text("This is some useful text.");               // Display some text (you can use a format strings too)

                ImGui::SliderFloat("float", &f, 0.0f, 1.0f);            // Edit 1 float using a slider from 0.0f to 1.0f

                if (ImGui::Button("Button"))                            // Buttons return true when clicked (most widgets return true when edited/activated)
                    counter++;
                ImGui::SameLine();
                ImGui::Text("counter = %d", counter);

                ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / io.Framerate, io.Framerate);
                ImGui::InputText("Input some text", &str);
            
                open = !open && ImGui::Button("Re-open");

                //TestManager::Instance().OnImGui();
                
            }
            ImGui::End();

            if (ImGui::Begin("Other", &open))
            {
                ImGui::Text("Entered text: %s", str.c_str());
                ImGui::Combo(var);
            }
            ImGui::End();
            
        }
        ImGui::Render();
        
        glClear(GL_COLOR_BUFFER_BIT);

        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
        {
            GLFWwindow* backup_current_context = glfwGetCurrentContext();
            ImGui::UpdatePlatformWindows();
            ImGui::RenderPlatformWindowsDefault();
            glfwMakeContextCurrent(backup_current_context);
        }

        /* Swap front and back buffers */
        glfwSwapBuffers(window);
    }

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}