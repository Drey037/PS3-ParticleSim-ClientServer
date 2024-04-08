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
#include <nlohmann/json.hpp>


// for convenience
using json = nlohmann::json;

#pragma comment(lib, "ws2_32.lib")

#include <imgui.h>
#include <imgui_impl_opengl3.h>
#include <GLFW/glfw3.h>
#include <imgui_impl_glfw.h>

#include "Particle.h"
#include "ParticleBatch.h"
#include "Ghost.h"

using namespace std;

static GLFWwindow* window = nullptr;
ImVec4 wallColor = ImVec4(1.0f, 1.0f, 0.0f, 1.0f);
ImVec4 particleColor = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);

std::vector<std::unique_ptr<ParticleBatch>> particleBatchList;
std::vector<std::string> allParticleStringList;
std::mutex particleListLock;
std::list<Ghost> clients;
//std::vector<Ghost> clients;
SOCKET serverSocket;
const int MAX_LOAD = 1000; // Assuming MAX_LOAD is a constant

const double MIN_VELOCITY = 5.0; // Define your minimum velocity here
const double MAX_VELOCITY = 500.0; // Define your maximum velocity here
const int PANEL_WIDTH = 1280; // Define your maximum velocity here
const int PANEL_HEIGHT = 720; // Define your maximum velocity here
int numClients;
int ParticleBatch::currentID = 0;
int Ghost::currentID = 0;




// Function to initialize ImGui
void InitImGui(GLFWwindow* window) {
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    ImGui::StyleColorsDark();
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 330 core");
}


// -------------------------------------- SERVER STUFF -------------------------------------------------

// This function is used to turn Coordinates into JSON
std::string serializeParticles(const std::vector <Particle>& particles, int batchID) {
    json j;
    j.push_back({ {"Type", 2} });
    for (const auto& particle : particles) {
        j.push_back({ {"BatchID", batchID}, {"X", particle.getX()}, {"Y", particle.getY()}, {"Theta", particle.getTheta()} , {"Velocity", particle.getVelocity()} });
    }
    std::string serializedJson = j.dump();
    serializedJson += '\n'; // Add newline character at the end
    return serializedJson;
}

// For client welcome, sends all needed particles
void clientWelcome(SOCKET socket) {
    for (auto& batch : allParticleStringList) {
        const char* data = batch.c_str();
        size_t dataLength = batch.size();

        int bytesSent = send(socket, data, dataLength, 0);
        if (bytesSent == SOCKET_ERROR) {
            std::cerr << "Error sending data: " << WSAGetLastError() << std::endl;
            closesocket(serverSocket);
            WSACleanup();
        }
    }
}

//This function is used to turn the json into string to be sent
void sendCoordinates(const std::string& serializedCoordinates) {
    cout << "Sent coordinates";
    const char* data = serializedCoordinates.c_str();
    size_t dataLength = serializedCoordinates.size();

    for (Ghost client : clients) {
        SOCKET clientSocket = client.getSocket();
        int bytesSent = send(clientSocket, data, dataLength, 0);
        if (bytesSent == SOCKET_ERROR) {
            std::cerr << "Error sending data: " << WSAGetLastError() << std::endl;
            closesocket(serverSocket);
            WSACleanup();
        }
    }
}

// SERVER LISTEN FOR CLIENTS
void listenForClients(SOCKET serverSocket) {
    while (true) {
        sockaddr_in clientAddr;
        int clientAddrSize = sizeof(clientAddr);
        SOCKET clientSocket = accept(serverSocket, reinterpret_cast<sockaddr*>(&clientAddr), &clientAddrSize);
        if (clientSocket != INVALID_SOCKET) {
            std::cout << "\nClient connected\n";
            clients.push_back(Ghost(1, 1, clientSocket));

            Ghost& newClient = clients.back(); // Get a reference to the newly added client
            std::cout << "Client ID: " << newClient.getID() << " connected\n"; // Print the client ID

            // Send a welcome message
            json j;
            j.push_back({ {"Type", 0} });
            j.push_back({ {"ID", newClient.getID()} }); // Use the new client's ID
            std::string serializedJson = j.dump();
            serializedJson += '\n'; // Add newline character at the end

            const char* data = serializedJson.c_str();
            size_t dataLength = serializedJson.size();

            int bytesSent = send(clientSocket, data, dataLength, 0);
            if (bytesSent == SOCKET_ERROR) {
                std::cerr << "Error sending welcome message: " << WSAGetLastError() << std::endl;
                closesocket(serverSocket);
                WSACleanup();
                continue;
            }
            std::cout << "Sent welcome message!\n";

            // Prepare and store particle data to be sent to the new client
            std::vector<std::string> pendingParticleData;
            {
                std::lock_guard<std::mutex> lock(particleListLock); // Ensure thread safety
                for (const auto& batch : particleBatchList) {
                    const ParticleBatch& batchDef = *batch;
                    std::vector<Particle> particles = batchDef.getParticles();
                    std::string serializedParticles = serializeParticles(particles, batchDef.getID());
                    pendingParticleData.push_back(serializedParticles);
                }
            }

            // Send pending particle data to the new client
            for (const auto& particleData : pendingParticleData) {
                size_t particleDataLength = particleData.size();
                int bytesSentToNewClient = send(clientSocket, particleData.c_str(), particleDataLength, 0);
                if (bytesSentToNewClient == SOCKET_ERROR) {
                    std::cerr << "Error sending particle data to new client: " << WSAGetLastError() << std::endl;
                    closesocket(serverSocket);
                    WSACleanup();
                    break; // Exit the loop to avoid sending further data on error
                }
            }
        }
    }
}


void removeClient(int clientID) {
    clients.remove_if([clientID](const Ghost& client) {
        return client.getID() == clientID;
        });
}




void sendNewPositionsToAll(int clientID, int newX, int newY) {
    json j;
    j.push_back({ {"Type", 1} });
    j.push_back({ {"ClientID", clientID}, {"X", newX}, {"Y", newY} });

    std::string serializedJson = j.dump();
    serializedJson += '\n'; // Add newline character at the end

    const char* data = serializedJson.c_str();
    size_t dataLength = serializedJson.size();

    for (auto& client : clients) {
        if (client.getID() != clientID)
            send(client.getSocket(), data, dataLength, 0);
    }
}
void processReceivedData(const std::string& data, std::list<Ghost>::iterator& it) {
    std::cout << "Received data: " << data << std::endl;
    try {
        json j = json::parse(data);

        if (j.is_array()) {
            for (const auto& element : j) {
                if (element.is_object()) {
                    int clientID = element["ClientID"];
                    int x = element["X"];
                    int y = element["Y"];
                    it->setX(x);
                    it->setY(y);
                    std::cout << "ClientID: " << clientID << ", X: " << x << ", Y: " << y << std::endl;
                    sendNewPositionsToAll(clientID, x, y);
                }
                else {
                    std::cerr << "Unexpected data type in JSON array" << std::endl;
                }
            }
        }
        else {
            std::cerr << "Received data is not a JSON array" << std::endl;
        }
    }
    catch (json::parse_error& e) {
        std::cerr << "JSON parse error: " << e.what() << std::endl;
        std::cerr << "Raw data: " << data << std::endl;
    }
    catch (json::type_error& e) {
        std::cerr << "JSON type error: " << e.what() << std::endl;
    }
}


// Function to handle receiving messages from clients
void receiveMessages() {
    fd_set readfds;
    struct timeval timeout;

    while (true) {
        FD_ZERO(&readfds); // Reset the set before each iteration
        int maxSocket = -1;
        for (auto& client : clients) {
            SOCKET sock = client.getSocket();
            if (sock == INVALID_SOCKET) {
                std::cerr << "Invalid socket encountered." << std::endl;
                continue; // Skip adding invalid sockets to the fd_set
            }
            FD_SET(sock, &readfds); // Add the socket to the set
            if (sock > maxSocket) {
                maxSocket = sock;
            }
        }

        // Set a timeout to prevent `select` from blocking indefinitely
        timeout.tv_sec = 1;  // 1 second
        timeout.tv_usec = 0;  // 0 microseconds

        int activity = select(maxSocket + 1, &readfds, nullptr, nullptr, &timeout);
        if (activity < 0) {
            int error = WSAGetLastError();
            if (error != WSAEINVAL) { // Ignore WSAEINVAL error
                std::cerr << "Select error: " << error << std::endl;
            }
            continue;
        }
        else if (activity == 0) {
            // No activity, continue to the next iteration
            continue;
        }

        for (auto it = clients.begin(); it != clients.end();) {
            SOCKET clientSocket = it->getSocket();
            if (FD_ISSET(clientSocket, &readfds)) {
                char buffer[1024] = { 0 };
                int bytesReceived = recv(clientSocket, buffer, sizeof(buffer), 0);
                if (bytesReceived > 0) {
                    buffer[bytesReceived] = '\0';
                    processReceivedData(buffer, it);
                    ++it;
                }
                else if (bytesReceived == 0) {
                    std::cout << "Client disconnected, closing socket." << std::endl;
                    closesocket(clientSocket);  // Close the socket
                    it = clients.erase(it);     // Remove the client from the list
                }
                else {
                    std::cerr << "recv failed with error: " << WSAGetLastError() << std::endl;
                    ++it;
                }
            }
            else {
                ++it;
            }
        }
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

            sendCoordinates(serializeParticles(pList, batchDef.getID()));

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
        sendCoordinates(serializeParticles(xList, lastParticleBatch.getID()));
    }
    std::sort(particleBatchList.begin(), particleBatchList.end(), ParticleBatchComparator());
}

void addParticlesDiffAngles(int n, int x, int y, double startTheta, double endTheta, double velocity) {
    double dTheta = (endTheta - startTheta) / static_cast<double>(n);
    double incTheta = startTheta;

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
                pList.push_back(Particle(x, y, velocity, incTheta));
                incTheta += dTheta;
            }

            sendCoordinates(serializeParticles(pList, batchDef.getID()));

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
                xList.push_back(Particle(x, y, velocity, incTheta));
                incTheta += dTheta;
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
        sendCoordinates(serializeParticles(xList, lastParticleBatch.getID()));
    }
    std::sort(particleBatchList.begin(), particleBatchList.end(), ParticleBatchComparator());
}

void addParticlesDiffVelocities(int n, int x, int y, double theta, double startVelocity, double endVelocity) {
    double dVelocity = (endVelocity - startVelocity) / static_cast<double>(n);
    double incVelo = startVelocity;
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
                pList.push_back(Particle(x, y, incVelo, theta));
                incVelo += dVelocity;
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
                xList.push_back(Particle(x, y, incVelo, theta));
                incVelo += dVelocity;
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
        const ParticleBatch& batchDef = *batch; // Ensure batchDef is const

        const std::vector<Particle>& particleList = batchDef.getParticles(); // Use const reference
        for (const Particle& particle : particleList) { // Use const reference for particles
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
    ImGui::SetNextWindowSize(ImVec2(1282, 722), ImGuiCond_Always);
    ImGui::Begin("Particle Spawn Area", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove);
    ImGui::Text("Current FPS: %.f", currentFramerate);
    DrawElements();
    ImGui::End();

    // Input Panel
    ImGui::SetNextWindowPos(ImVec2(1282, 0), ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(260, 722), ImGuiCond_Always);
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
                double angle_dob = std::stod(angle_str);
                double velocity_dob = std::stod(velocity_str);

                // Check if coordinates are within the window bounds
                if (startX_int < 0 || startX_int > PANEL_WIDTH || startY_int < 0 || startY_int > PANEL_HEIGHT ||
                    endX_int < 0 || endX_int > PANEL_WIDTH || endY_int < 0 || endY_int > PANEL_HEIGHT) {

                    std::cout << "Invalid Coordinates!";
                    ImGui::End();
                    return;
                }

                // Check if theta is within the valid range
                if (angle_dob < 0 || angle_dob > 360) {

                    std::cout << "Theta must be between 0 and 360 degrees!";
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

            std::string n_str(n);
            std::string X_str(startX);
            std::string Y_str(startY);
            std::string startAngle_str(startAngle);
            std::string endAngle_str(endAngle);
            std::string velocity_str(startVelocity);

            try {
                int n_int = std::stoi(n_str);
                int X_int = std::stoi(X_str);
                int Y_int = std::stoi(Y_str);
                double startAngle_dob = std::stod(startAngle_str);
                double endAngle_dob = std::stod(endAngle_str);
                double velocity_dob = std::stod(velocity_str);

                // Check if coordinates are within the window bounds
                if (X_int < 0 || X_int > PANEL_WIDTH || Y_int < 0 || Y_int > PANEL_HEIGHT) {

                    std::cout << "Invalid Coordinates!";

                    ImGui::End();
                    return;
                }

                // Check if theta is within the valid range
                if (startAngle_dob < 0 || startAngle_dob > 360 || endAngle_dob < 0 || endAngle_dob > 360) {

                    std::cout << "Theta must be between 0 and 360 degrees!";

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

                    ImGui::End();
                    return;
                }


                // SPAWN PARTICLES
                addParticlesDiffAngles(n_int, X_int, Y_int, startAngle_dob, endAngle_dob, velocity_dob);
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
            std::cout << "Add Particles button clicked for different velocities" << std::endl;

            std::string n_str(n);
            std::string X_str(startX);
            std::string Y_str(startY);
            std::string angle_str(startAngle);
            std::string startVelocity_str(startVelocity);
            std::string endVelocity_str(endVelocity);

            try {
                int n_int = std::stoi(n_str);
                int X_int = std::stoi(X_str);
                int Y_int = std::stoi(Y_str);
                double angle_dob = std::stod(angle_str);
                double startVelocity_dob = std::stod(startVelocity_str);
                double endVelocity_dob = std::stod(endVelocity_str);

                // Check if coordinates are within the window bounds
                if (X_int < 0 || X_int > PANEL_WIDTH || Y_int < 0 || Y_int > PANEL_HEIGHT) {
                    std::cout << "Invalid Coordinates!";

                    ImGui::End();
                    return;
                }

                // Check if angle is within the valid range
                if (angle_dob < 0 || angle_dob > 360) {
                    std::cout << "Theta must be between 0 and 360 degrees!";


                    ImGui::End();
                    return;
                }

                // Check if velocity is non-negative
                if (startVelocity_dob < MIN_VELOCITY || startVelocity_dob > MAX_VELOCITY || endVelocity_dob < MIN_VELOCITY || endVelocity_dob > MAX_VELOCITY) {
                    std::string min = std::to_string(MIN_VELOCITY);
                    std::string max = std::to_string(MAX_VELOCITY);
                    std::string msg = "Velocities must be between " + min + " and " + max + ".";
                    const char* message = msg.c_str();

                    std::cout << msg;

                    ImGui::End();
                    return;
                }


                // SPAWN PARTICLES
                addParticlesDiffVelocities(n_int, X_int, Y_int, angle_dob, startVelocity_dob, endVelocity_dob);
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

static void GLFWErrorCallback(int error, const char* description) {
    std::cout << "GLFW Error " << description << " code: " << error << std::endl;
}



int main() {
    serverStart();

    // Create threads for listening, receiving, and sending
    thread listenThread(listenForClients, serverSocket);
    thread receiveThread(receiveMessages);


    if (!glfwInit()) {
        std::cout << "Failed to initialize GLFW" << std::endl;
        std::cin.get();
    }

    glfwSetErrorCallback(GLFWErrorCallback);

    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

    GLFWwindow* window = glfwCreateWindow(1540, 722, "Particle Simulator", nullptr, nullptr);
    if (!window) {
        std::cerr << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }

    particleBatchList.emplace_back(std::make_unique<ParticleBatch>());

    glfwMakeContextCurrent(window);
    glfwSwapInterval(0); // Disable Vsync

    InitImGui(window);

    // FPS Stuff
    const double targetFPS = 60.0; // Set a fixed target FPS of 60
    double targetFrameTime = 1.0 / targetFPS; // Target time per frame (60 FPS)

    auto lastFrameTime = std::chrono::steady_clock::now();
    double frameTimeAccumulator = 0.0;
    double currentFramerate = 0.0;
    int frameCount = 0;
    double lastFPSUpdateTime = glfwGetTime();

    while (!glfwWindowShouldClose(window)) {
        auto currentFrameTime = std::chrono::steady_clock::now();
        std::chrono::duration<double> elapsedTime = currentFrameTime - lastFrameTime;
        double deltaTime = elapsedTime.count();

        // Update game state
        glfwPollEvents();
        RenderImGui(currentFramerate);

        // Update particles
        std::vector<std::future<void>> futures;
        std::lock_guard<std::mutex> lock(particleListLock);

        for (auto& batch : particleBatchList) {
            ParticleBatch& batchDef = *batch;
            futures.push_back(std::async(std::launch::async, [&batchDef, deltaTime]() {
                batchDef.updateParticles(deltaTime);
                }));
        }

        for (auto& future : futures) {
            future.wait();
        }

        // Rendering
        ImGui::Render();
        int display_w, display_h;
        glfwGetFramebufferSize(window, &display_w, &display_h);
        glViewport(0, 0, display_w, display_h);
        glClearColor(0.45f, 0.55f, 0.60f, 1.00f);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        glfwSwapBuffers(window);

        // Frame rate control
        auto endFrameTime = std::chrono::steady_clock::now();
        std::chrono::duration<double> frameDuration = endFrameTime - currentFrameTime;

        frameTimeAccumulator += frameDuration.count();
        frameCount++;
        if (glfwGetTime() - lastFPSUpdateTime >= 1.0) {
            currentFramerate = frameCount / frameTimeAccumulator;
            frameCount = 0;
            frameTimeAccumulator = 0.0;
            lastFPSUpdateTime = glfwGetTime();
        }

        // Correctly calculate and apply the sleep duration
        double frameDurationInSeconds = frameDuration.count();
        if (frameDurationInSeconds < targetFrameTime) {
            double sleepDurationInSeconds = targetFrameTime - frameDurationInSeconds;
            std::this_thread::sleep_for(std::chrono::duration<double>(sleepDurationInSeconds));
        }

        lastFrameTime = currentFrameTime;
    }

    // Wait for threads to finish (this will never happen in this example, but it's here for completeness)
    listenThread.join();
    receiveThread.join();

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