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
#include <iostream>
#include <winsock2.h>
#include <ws2tcpip.h>

#pragma comment(lib, "ws2_32.lib")

#include <imgui.h>
#include <imgui_impl_opengl3.h>
#include <GLFW/glfw3.h>
#include <imgui_impl_glfw.h>

#include "Particle.h"
#include "ParticleBatch.h"
// #include "Ghost.h"

using namespace std;

static GLFWwindow* window = nullptr;
ImVec4 wallColor = ImVec4(1.0f, 1.0f, 0.0f, 1.0f);
ImVec4 particleColor = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);

std::vector<std::unique_ptr<ParticleBatch>> particleBatchList;
std::mutex particleListLock;
std::vector<SOCKET> clientSockets;
SOCKET serverSocket;
const int MAX_LOAD = 100; // Assuming MAX_LOAD is a constant

const double MIN_VELOCITY = 5.0; // Define your minimum velocity here
const double MAX_VELOCITY = 30.0; // Define your maximum velocity here
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


// ----------------------ADDING PARTICLES---------------------------------------------
struct ParticleBatchComparator {
    bool operator()(const std::unique_ptr<ParticleBatch>& batch1Ptr, const std::unique_ptr<ParticleBatch>& batch2Ptr) const {
        // Dereference the unique_ptr to get the ParticleBatch objects
        const ParticleBatch& batch1 = *batch1Ptr;
        const ParticleBatch& batch2 = *batch2Ptr;

        // Now you can compare the ParticleBatch objects
        return batch1.getNumParticles() < batch2.getNumParticles();
    }
};



void addParticlesDiffPoints(int n, int startX, int endX, int startY, int endY, double theta, double velocity) {
    double totalDistance = std::sqrt(std::pow(endX - startX, 2) + std::pow(endY - startY, 2));
    double increment = totalDistance / (n - 1);
    double unitVectorX = (endX - startX) / totalDistance;
    double unitVectorY = (endY - startY) / totalDistance;

    double currentX = startX;
    double currentY = startY;

    int remainingCount = n;

    // Assuming particleBatchList is a vector of ParticleBatch objects
    for (auto& batch : particleBatchList) {

        ParticleBatch& batchDef = *batch;

        if (batchDef.isFull())
            break;
        else {
            std::vector<Particle> pList;
            int numNeeded = MAX_LOAD - batchDef.getNumParticles();

            if (numNeeded > remainingCount) {
                numNeeded = remainingCount;
                remainingCount = 0;
            }
            else
                remainingCount -= numNeeded;

            for (int i = 0; i < numNeeded; i++) {
                pList.push_back(Particle(static_cast<int>(std::round(currentX)), static_cast<int>(std::round(currentY)), velocity, theta));
                currentX += increment * unitVectorX;
                currentY += increment * unitVectorY;
            }

            // Assuming particleListLock is a std::mutex
            std::lock_guard<std::mutex> lock(particleListLock);
            batchDef.addNewParticles(pList, numNeeded);
        }
    }

    while (remainingCount > 0) {
        std::vector<Particle> xList;

        int added = 0;
        for (int i = 0; i < MAX_LOAD; i++) {
            if (remainingCount > 0) {
                xList.push_back(Particle(static_cast<int>(std::round(currentX)), static_cast<int>(std::round(currentY)), velocity, theta));
                currentX += increment * unitVectorX;
                currentY += increment * unitVectorY;
                remainingCount--;

                added = i;
            }
            else
                break;
        }
        std::lock_guard<std::mutex> lock(particleListLock);
        particleBatchList.emplace_back(std::make_unique<ParticleBatch>());

        std::unique_ptr<ParticleBatch>& lastParticleBatchPtr = particleBatchList.back();

        ParticleBatch& lastParticleBatch = *lastParticleBatchPtr;

        lastParticleBatch.clearParticles();
        lastParticleBatch.addNewParticles(xList, added);
    }
    std::sort(particleBatchList.begin(), particleBatchList.end(), ParticleBatchComparator());
}

// ---------------------- END ADDING PARTICLES---------------------------------------------


static void DrawElements() {
    ImDrawList* draw_list = ImGui::GetWindowDrawList();

    // Draw particles
    for (auto& batch : particleBatchList) {
        ParticleBatch& batchDef = *batch;

        std::vector<Particle>& particleList = batchDef.getParticles();
        for (Particle& particle : particleList) {
            ImVec2 screenPos = ImVec2(particle.getX(), 720 - particle.getY());

            float size = static_cast<float>(particle.getSize()); // Cast the size to float
            draw_list->AddCircleFilled(screenPos, size, IM_COL32(255, 255, 255, 255));
        }
    }
}

// Function to render ImGui
void RenderImGui(double currentFramerate) {
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    // Particle Spawn Area
    ImGui::SetNextWindowPos(ImVec2(0, 0), ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(1280, 720), ImGuiCond_Always);
    ImGui::Begin("Particle Spawn Area", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove);
    ImGui::Text("Current FPS: %.f", currentFramerate);
    DrawElements();
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

            std::string n_str(n);
            std::string startX_str(startX);
            std::string startY_str(startY);
            std::string endX_str(endX);
            std::string endY_str(endY);
            std::string angle_str(startAngle);
            std::string velocity_str(startVelocity);

            try {
                int n_int = std::stoi(n_str);
                int startX_int = std::stoi(startX_str);
                int startY_int = std::stoi(startY_str);
                int endX_int = std::stoi(endX_str);
                int endY_int = std::stoi(endY_str);
                int angle_dob = std::stod(angle_str);
                int velocity_dob = std::stod(velocity_str);

                // Check if coordinates are within the window bounds
                if (startX_int < 0 || startX_int > PANEL_WIDTH || startY_int < 0 || startY_int > PANEL_HEIGHT ||
                    endX_int < 0 || endX_int > PANEL_WIDTH|| endY_int < 0 || endY_int > PANEL_HEIGHT) {

                    std::cout << "Invalid Coordinates!";

                    ShowMessagePopup("Error!", "Invalid coordinates!");
                    ImGui::End();
                    return;
                }

                // Check if theta is within the valid range
                if (angle_dob < 0 || angle_dob > 360) {

                    std::cout << "Theta must be between 0 and 360 degrees!";

                    ShowMessagePopup("Error!", "Theta must be between 0 and 360 degrees!");
                    ImGui::End();
                    return;
                }

                // Check if velocity is non-negative
                if (velocity_dob < MIN_VELOCITY || velocity_dob > MAX_VELOCITY) {
                    std::string min = std::to_string(MIN_VELOCITY);
                    std::string max = std::to_string(MAX_VELOCITY);
                    std::string msg = "Velocity must be between " + min + " and " + max + ".";
                    const char* message = msg.c_str();

                    std::cout << msg;

                    ShowMessagePopup("Error!", message);
                    ImGui::End();
                    return;
                }


                // SPAWN PARTICLES
                addParticlesDiffPoints(n_int, startX_int, endX_int, startY_int, endY_int, angle_dob, velocity_dob);
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

                    ImGui::End();
                    return;
                }

                // Check if theta is within the valid range
                if (startAngle_int < 0 || startAngle_int > 360 || endAngle_int < 0 || endAngle_int > 360) {
                    ShowMessagePopup("Error!", "Theta must be between 0 and 360 degrees!");

                    ImGui::End();
                    return;
                }

                // Check if velocity is non-negative
                if (velocity_int < MIN_VELOCITY || velocity_int > MAX_VELOCITY) {
                    std::string min = std::to_string(MIN_VELOCITY);
                    std::string max = std::to_string(MAX_VELOCITY);
                    std::string msg = "Velocity must be between " + min + " and " + max + ".";
                    const char* message = msg.c_str();

                    ShowMessagePopup("Error!", message);

                    ImGui::End();
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

                    ImGui::End();
                    return;
                }

                // Check if angle is within the valid range
                if (angle_int < 0 || angle_int > 360) {
                    ShowMessagePopup("Error!", "Theta must be between 0 and 360 degrees!");

                    ImGui::End();
                    return;
                }

                // Check if velocity is non-negative
                if (startVelocity_int < MIN_VELOCITY || startVelocity_int < MAX_VELOCITY || endVelocity_int < MIN_VELOCITY || endVelocity_int < MAX_VELOCITY) {
                    std::string min = std::to_string(MIN_VELOCITY);
                    std::string max = std::to_string(MAX_VELOCITY);
                    std::string msg = "Velocities must be between " + min + " and " + max + ".";
                    const char* message = msg.c_str();

                    ShowMessagePopup("Error!", message);

                    ImGui::End();
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
}

// -------------------------------------- SERVER STUFF -------------------------------------------------

void listenForClients(SOCKET serverSocket) {
    while (true) {
        sockaddr_in clientAddr;
        int clientAddrSize = sizeof(clientAddr);
        SOCKET clientSocket = accept(serverSocket, reinterpret_cast<sockaddr*>(&clientAddr), &clientAddrSize);
        if (clientSocket != INVALID_SOCKET) {
            std::cout << "Client connected\n";
            clientSockets.push_back(clientSocket);
        }
    }
}

// Function to handle receiving messages from clients
void receiveMessages(const std::vector<SOCKET>& clientSockets) {
    while (true) {
        for (SOCKET clientSocket : clientSockets) {
            char buffer[1024];
            int bytesReceived = recv(clientSocket, buffer, sizeof(buffer), 0);
            if (bytesReceived > 0) {
                std::cout << "Received: " << std::string(buffer, bytesReceived) << "\n";
            }
        }
    }
}

// Function to handle sending messages to clients
void sendMessages(const std::vector<SOCKET>& clientSockets) {
    while (true) {
        // Example: Sending a message to all clients
        std::string message = "Hello from server!";
        for (SOCKET clientSocket : clientSockets) {
            send(clientSocket, message.c_str(), message.size(), 0);
        }
        // Sleep for a while to avoid sending too frequently
        std::this_thread::sleep_for(std::chrono::seconds(5));
    }
}

int serverStart() {
    // Initialize Winsock
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        std::cerr << "Error: WSAStartup failed\n";
        return 1;
    }

    // Create a socket
    serverSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (serverSocket == INVALID_SOCKET) {
        std::cerr << "Error: Socket creation failed\n";
        WSACleanup();
        return 1;
    }

    // Set up the server address
    sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(12345); // Port number of the server
    serverAddr.sin_addr.s_addr = htonl(INADDR_ANY); // Listen on all interfaces

    // Bind the socket to the server address
    if (::bind(serverSocket, reinterpret_cast<sockaddr*>(&serverAddr), sizeof(serverAddr)) == SOCKET_ERROR) {
        std::cerr << "Error: Bind failed\n";
        closesocket(serverSocket);
        WSACleanup();
        return 1;
    }

    // Listen for incoming connections
    if (listen(serverSocket, SOMAXCONN) == SOCKET_ERROR) {
        std::cerr << "Error: Listen failed\n";
        closesocket(serverSocket);
        WSACleanup();
        return 1;
    }

    std::cout << "Server is listening on port 12345\n";

    return 0;
}
// ---------------------------END SERVER STUFF ----------------------------------------

int main() {
    serverStart();

    // Create threads for listening, receiving, and sending
    std::thread listenThread(listenForClients, serverSocket);
    std::thread receiveThread(receiveMessages, std::ref(clientSockets));
    std::thread sendThread(sendMessages, std::ref(clientSockets));

    // FPS Stuff
    double frameTime = 0.0; // Time since the last frame
    double targetFrameTime = 1.0 / 60.0; // Target time per frame (60 FPS)
    double updateInterval = 0.5; // Interval for updating particles (0.5 seconds)
    double lastUpdateTime = 0.0; // Last time particles were updated
    double lastFPSUpdateTime = 0.0; // Last time the framerate was updated

    const double targetFPS = 60.0;
    const std::chrono::duration<double> targetFrameDuration = std::chrono::duration<double>(1.0 / targetFPS);

    std::chrono::steady_clock::time_point prevTime = std::chrono::steady_clock::now();

    const double timeStep = 1.0 / targetFPS; // Time step for updates
    double accumulator = 0.0; // Accumulates elapsed time

    double currentFramerate = 0.0;
    double lastUIUpdateTime = 0.0;



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

    particleBatchList.emplace_back(std::make_unique<ParticleBatch>());

    glfwMakeContextCurrent(window);
    glfwSwapInterval(0); // Disable Vsync

    InitImGui(window);

    while (!glfwWindowShouldClose(window)) {
        double currentTime = glfwGetTime();
        frameTime = currentTime - lastUpdateTime;

        std::chrono::steady_clock::time_point currentTimeForDelta = std::chrono::steady_clock::now();
        std::chrono::duration<double> elapsedTime = currentTimeForDelta - prevTime;
        prevTime = currentTimeForDelta;

        accumulator += frameTime;

        if (elapsedTime > std::chrono::seconds(1)) {
            elapsedTime = std::chrono::duration<double>(1.0 / targetFPS);
        }

        double deltaTime = elapsedTime.count();

        glfwPollEvents();

        glClearColor(0.45f, 0.55f, 0.60f, 1.00f);
        glClear(GL_COLOR_BUFFER_BIT);

        RenderImGui(currentFramerate);

        while (accumulator >= timeStep) {
            std::vector<std::future<void>> futures;
            std::lock_guard<std::mutex> lock(particleListLock);

            for (auto& batch : particleBatchList) {
                ParticleBatch& batchDef = *batch;

                futures.push_back(std::async(std::launch::async, [&batchDef, timeStep]() {
                    batchDef.updateParticles(timeStep);
                    }));
            }
            
            futures.push_back(std::async(std::launch::async, DrawElements));

            for (auto& future : futures) {
                future.wait();
            }
            accumulator -= timeStep;
        }

        if (frameTime >= targetFrameTime) {
            lastUpdateTime = currentTime;
        }

        if (currentTime - lastFPSUpdateTime >= updateInterval) {
            currentFramerate = 1.0 / frameTime;
            lastFPSUpdateTime = currentTime;
        }
  
        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        glfwSwapBuffers(window);
    }

    // Wait for threads to finish (this will never happen in this example, but it's here for completeness)
    listenThread.join();
    receiveThread.join();
    sendThread.join();

    // Close the server socket when done
    closesocket(serverSocket);
    WSACleanup();

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}
