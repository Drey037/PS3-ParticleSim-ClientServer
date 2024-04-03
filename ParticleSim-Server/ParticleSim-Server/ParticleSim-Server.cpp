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

const double MIN_VELOCITY = 0.0; // Define your minimum velocity here
const double MAX_VELOCITY = 100.0; // Define your maximum velocity here
const int PANEL_WIDTH = 1280; // Define your maximum velocity here
const int PANEL_HEIGHT = 720; // Define your maximum velocity here

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


    ImGui::InputText("n", n, IM_ARRAYSIZE(n));
    switch (current_item) {
    case 0: // Different Points
        ImGui::InputText("Start X", startX, IM_ARRAYSIZE(startX));
        ImGui::InputText("Start Y", startY, IM_ARRAYSIZE(startY));
        ImGui::InputText("End X", endX, IM_ARRAYSIZE(endX));
        ImGui::InputText("End Y", endY, IM_ARRAYSIZE(endY));
        ImGui::InputText("Angle", startAngle, IM_ARRAYSIZE(startAngle));
        ImGui::InputText("Velocity", startVelocity, IM_ARRAYSIZE(startVelocity));
        if (ImGui::Button("Add Particles")) {
            // Handle button click
            std::cout << "Add Particles button clicked for different points" << std::endl;

            std::string startX_str(startX);
            std::string startY_str(startY);
            std::string endX_str(endX);
            std::string endY_str(endY);
            std::string angle_str(startAngle);
            std::string velocity_str(startVelocity);

            try {
                int startX_int = std::stoi(startX_str);
                int startY_int = std::stoi(startY_str);
                int endX_int = std::stoi(endX_str);
                int endY_int = std::stoi(endY_str);
                int angle_int = std::stoi(angle_str);
                int velocity_int = std::stoi(velocity_str);

                // Check if coordinates are within the window bounds
                if (startX_int < 0 || startX_int > PANEL_WIDTH || startY_int < 0 || startY_int > PANEL_HEIGHT ||
                    endX_int < 0 || endX_int > PANEL_WIDTH|| endY_int < 0 || endY_int > PANEL_HEIGHT) {
                    ShowMessagePopup("Error!", "Invalid coordinates!");
                    return;
                }

                // Check if theta is within the valid range
                if (angle_int < 0 || angle_int > 360) {
                    ShowMessagePopup("Error!", "Theta must be between 0 and 360 degrees!");
                    return;
                }

                // Check if velocity is non-negative
                if (velocity_int < MIN_VELOCITY || velocity_int < MAX_VELOCITY) {
                    std::string min = std::to_string(MIN_VELOCITY);
                    std::string max = std::to_string(MAX_VELOCITY);
                    std::string msg = "Velocity must be between " + min + " and " + max + ".";
                    const char* message = msg.c_str();

                    ShowMessagePopup("Error!", message);
                    return;
                }


                // SPAWN PARTICLES
            }
            catch (const std::invalid_argument& e) {
                std::cout << "Invalid argument: " << e.what() << std::endl;
            }
            catch (const std::out_of_range& e) {
                std::cout << "Out of range: " << e.what() << std::endl;
            }
        }

        break;
    case 1: // Different Angles
        ImGui::InputText("X", startX, IM_ARRAYSIZE(startX));
        ImGui::InputText("Y", startY, IM_ARRAYSIZE(startY));
        ImGui::InputText("Start Angle", startAngle, IM_ARRAYSIZE(startAngle));
        ImGui::InputText("End Angle", endAngle, IM_ARRAYSIZE(endAngle));
        ImGui::InputText("Velocity", startVelocity, IM_ARRAYSIZE(startVelocity));

        if (ImGui::Button("Add Particles")) {
            // Handle button click
            std::cout << "Add Particles button clicked for different angles" << std::endl;

            std::string X_str(startX);
            std::string Y_str(startY);
            std::string startAngle_str(startAngle);
            std::string endAngle_str(endAngle);
            std::string velocity_str(startVelocity);

            try {
                int X_int = std::stoi(X_str);
                int Y_int = std::stoi(Y_str);
                int startAngle_int = std::stoi(startAngle_str);
                int endAngle_int = std::stoi(endAngle_str);
                int velocity_int = std::stoi(velocity_str);

                // Check if coordinates are within the window bounds
                if (X_int < 0 || X_int > PANEL_WIDTH || Y_int < 0 || Y_int > PANEL_HEIGHT) {
                    ShowMessagePopup("Error!", "Invalid coordinates!");
                    return;
                }

                // Check if theta is within the valid range
                if (startAngle_int < 0 || startAngle_int > 360 || endAngle_int < 0 || endAngle_int > 360) {
                    ShowMessagePopup("Error!", "Theta must be between 0 and 360 degrees!");
                    return;
                }

                // Check if velocity is non-negative
                if (velocity_int < MIN_VELOCITY || velocity_int < MAX_VELOCITY) {
                    std::string min = std::to_string(MIN_VELOCITY);
                    std::string max = std::to_string(MAX_VELOCITY);
                    std::string msg = "Velocity must be between " + min + " and " + max + ".";
                    const char* message = msg.c_str();

                    ShowMessagePopup("Error!", message);
                    return;
                }


                // SPAWN PARTICLES
            }
            catch (const std::invalid_argument& e) {
                std::cout << "Invalid argument: " << e.what() << std::endl;
            }
            catch (const std::out_of_range& e) {
                std::cout << "Out of range: " << e.what() << std::endl;
            }
        }

        break;
    case 2: // Different Velocities
        ImGui::InputText("X", startX, IM_ARRAYSIZE(startX));
        ImGui::InputText("Y", startY, IM_ARRAYSIZE(startY));
        ImGui::InputText("Angle", startAngle, IM_ARRAYSIZE(startAngle));
        ImGui::InputText("Start Velocity", startVelocity, IM_ARRAYSIZE(startVelocity));
        ImGui::InputText("End Velocity", endVelocity, IM_ARRAYSIZE(endVelocity));

        if (ImGui::Button("Add Particles")) {
            // Handle button click
            std::cout << "Add Particles button clicked for different angles" << std::endl;

            std::string X_str(startX);
            std::string Y_str(startY);
            std::string angle_str(startAngle);
            std::string startVelocity_str(startVelocity);
            std::string endVelocity_str(endVelocity);

            try {
                int X_int = std::stoi(X_str);
                int Y_int = std::stoi(Y_str);
                int angle_int = std::stoi(angle_str);
                int startVelocity_int = std::stoi(startVelocity_str);
                int endVelocity_int = std::stoi(endVelocity_str);

                // Check if coordinates are within the window bounds
                if (X_int < 0 || X_int > PANEL_WIDTH || Y_int < 0 || Y_int > PANEL_HEIGHT) {
                    ShowMessagePopup("Error!", "Invalid coordinates!");
                    return;
                }

                // Check if angle is within the valid range
                if (angle_int < 0 || angle_int > 360) {
                    ShowMessagePopup("Error!", "Theta must be between 0 and 360 degrees!");
                    return;
                }

                // Check if velocity is non-negative
                if (startVelocity_int < MIN_VELOCITY || startVelocity_int < MAX_VELOCITY || endVelocity_int < MIN_VELOCITY || endVelocity_int < MAX_VELOCITY) {
                    std::string min = std::to_string(MIN_VELOCITY);
                    std::string max = std::to_string(MAX_VELOCITY);
                    std::string msg = "Velocities must be between " + min + " and " + max + ".";
                    const char* message = msg.c_str();

                    ShowMessagePopup("Error!", message);
                    return;
                }


                // SPAWN PARTICLES
            }
            catch (const std::invalid_argument& e) {
                std::cout << "Invalid argument: " << e.what() << std::endl;
            }
            catch (const std::out_of_range& e) {
                std::cout << "Out of range: " << e.what() << std::endl;
            }
        }

        break;
    }
    

    ImGui::End();

    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

void ShowMessagePopup(const char* title, const char* message) {
    // Open the modal window
    ImGui::OpenPopup(title);

    // Create the modal window
    if (ImGui::BeginPopupModal(title, nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::Text("%s", message);

        // Provide a button to close the modal window
        if (ImGui::Button("OK")) {
            ImGui::CloseCurrentPopup();
        }

        ImGui::EndPopup();
    }
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
