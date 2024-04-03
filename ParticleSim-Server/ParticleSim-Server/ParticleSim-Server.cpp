#include <iostream>
#include <stdlib.h>
#include <thread>
#include <vector>
#include <chrono>
#include <string>
#include <ctime>
#include <cmath>
#include <random>
#include <future>

#include <imgui.h>
#include <imgui_impl_opengl3.h>
#include <GLFW/glfw3.h>
#include <imgui_impl_glfw.h>

// #include "Particle.h"
// #include "ParticleBatch.h"
// #include "Ghost.h"

using namespace std;

static GLFWwindow* window = nullptr;
float PI = 3.14159265359;
ImVec4 wallColor = ImVec4(1.0f, 1.0f, 0.0f, 1.0f);
ImVec4 particleColor = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);

// Function to initialize ImGui
void InitImGui(GLFWwindow* window) {
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    ImGui::StyleColorsDark();
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 330 core");
}

// Function to render ImGui
void RenderImGui() {
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    // Particle Spawn Area
    ImGui::SetNextWindowPos(ImVec2(0, 0), ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(1280, 720), ImGuiCond_Always);
    ImGui::Begin("Particle Spawn Area", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove);
    ImGui::End();

    // Input Panel
    ImGui::SetNextWindowPos(ImVec2(1280, 0), ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(260, 720), ImGuiCond_Always);
    ImGui::Begin("Input Panel", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove);

    // Dropdown
    static const char* items[] = { "Different Points", "Different Angles", "Different Velocities" };
    static int current_item = 0;
    ImGui::Combo("Options", &current_item, items, IM_ARRAYSIZE(items));

    // Text Fields
    static char n[32] = "";
    static char startX[32] = "";
    static char startY[32] = "";
    static char endX[32] = "";
    static char endY[32] = "";
    static char startAngle[32] = "";
    static char startVelocity[32] = "";
    static char endAngle[32] = "";
    static char endVelocity[32] = "";


    ImGui::InputText("n (No. of Particles)", n, IM_ARRAYSIZE(n));
    switch (current_item) {
    case 0: // Different Points
        ImGui::InputText("Start X", startX, IM_ARRAYSIZE(startX));
        ImGui::InputText("Start Y", startY, IM_ARRAYSIZE(startY));
        ImGui::InputText("End X", endX, IM_ARRAYSIZE(endX));
        ImGui::InputText("End Y", endY, IM_ARRAYSIZE(endY));
        ImGui::InputText("Angle", startAngle, IM_ARRAYSIZE(startAngle));
        ImGui::InputText("Velocity", startVelocity, IM_ARRAYSIZE(startVelocity));
        break;
    case 1: // Different Angles
        ImGui::InputText("X", startX, IM_ARRAYSIZE(startX));
        ImGui::InputText("Y", startY, IM_ARRAYSIZE(startY));
        ImGui::InputText("Start Angle", startAngle, IM_ARRAYSIZE(startAngle));
        ImGui::InputText("End Angle", endAngle, IM_ARRAYSIZE(endAngle));
        ImGui::InputText("Velocity", startVelocity, IM_ARRAYSIZE(startVelocity));
        break;
    case 2: // Different Velocities
        ImGui::InputText("X", startX, IM_ARRAYSIZE(startX));
        ImGui::InputText("Y", startY, IM_ARRAYSIZE(startY));
        ImGui::InputText("Angle", startAngle, IM_ARRAYSIZE(startAngle));
        ImGui::InputText("Start Velocity", startVelocity, IM_ARRAYSIZE(startVelocity));
        ImGui::InputText("End Velocity", endVelocity, IM_ARRAYSIZE(endVelocity));
        break;
    }

    // Button
    if (ImGui::Button("Add Particles")) {
        // Handle button click
        std::cout << "Add Particles button clicked." << std::endl;
    }

    ImGui::End();

    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

int main() {
    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW" << std::endl;
        return -1;
    }

    GLFWwindow* window = glfwCreateWindow(1540, 720, "Particle Simulator", nullptr, nullptr);
    if (!window) {
        std::cerr << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }

    glfwMakeContextCurrent(window);
    glfwSwapInterval(1); // Enable vsync

    InitImGui(window);

    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();

        glClearColor(0.45f, 0.55f, 0.60f, 1.00f);
        glClear(GL_COLOR_BUFFER_BIT);

        RenderImGui();

        glfwSwapBuffers(window);
    }

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}
