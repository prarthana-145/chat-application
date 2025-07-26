#include <iostream>
#include <WinSock2.h> // Include Windows socket library
#pragma comment(lib, "ws2_32.lib") // Link winsock library
#include <WS2tcpip.h>
#include<tchar.h>
#include<thread>
#include<vector>
#include <algorithm> 
#include <mutex>  
using namespace std;
mutex clients_mutex;

void initializeWinsock() {
    WSADATA wsaData;
    int result = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (result != 0) {
        cout << "WSAStartup failed: " << result << endl;
        exit(1);
    }
    cout << "Winsock initialized successfully!" << endl;
}

void clientHandler(SOCKET clientSocket, vector<SOCKET>& clients) {
    char buffer[1024];
    cout << "Client connected!" << endl;



    // Step 6: Clean up resources
    while (true) {
        // Clear buffer to avoid garbage data
        memset(buffer, 0, sizeof(buffer));

        // Receive data from the client
        int bytesrecvd = recv(clientSocket, buffer, sizeof(buffer) - 1, 0);
        if (bytesrecvd <= 0) {
            cout << "Client disconnected or error occurred." << endl;
            break;
        }

        // Null terminate the buffer
        buffer[bytesrecvd] = '\0';
        string message(buffer, bytesrecvd);
        cout << "message from client: " << message << endl;

        {  // Scope the lock properly
            lock_guard<mutex> guard(clients_mutex); // <<< FIX: Lock the mutex
            for (auto client : clients) {
                //client 1 ne server ko bheja msg ki client 2 to 5 ko bhejo
                //so we need to make sure client 1 ke paas msg vaapis na chale jaaye isliye remove that in loop(no echo)
                if (client != clientSocket) {
                    int sendResult = send(client, message.c_str(), message.length(), 0);
                    if (sendResult == SOCKET_ERROR) {
                        cout << "Error sending to a client: " << WSAGetLastError() << endl;
                    }
                }
            }
        }
    }

    {  // Scope the lock properly
        lock_guard<mutex> guard(clients_mutex); // <<< FIX: Lock the mutex
        //once we break before closing socket usse vector se remove krna h
        auto it = find(clients.begin(), clients.end(), clientSocket);
        if (it != clients.end())
            clients.erase(it);
    }

    // Close the client socket
    closesocket(clientSocket);
}

int main() {
    initializeWinsock();

    // Step 2: Create a socket
    SOCKET serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSocket == INVALID_SOCKET) {
        cout << "Socket creation failed: " << WSAGetLastError() << endl;
        WSACleanup();
        exit(1);
    }
    cout << "Socket created successfully!" << endl;

    // Step 3: Bind the socket to an IP and port
    sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY; // Use any available network interface
    serverAddr.sin_port = htons(12345); // Port number

    //covert the ip adress (0.0.0.0) put it inside the sin family in binary format
    if (inet_pton(AF_INET, "0.0.0.0", &serverAddr.sin_addr) != 1) {
        cout << "setting address structure failed" << endl;
        WSACleanup();
        return 1;
    }
    //bind
    if (bind(serverSocket, (sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
        cout << "Bind failed: " << WSAGetLastError() << endl;
        closesocket(serverSocket);
        WSACleanup();
        exit(1);
    }
    cout << "Socket bound to port 12345!" << endl;

    // Step 4: Start listening for incoming connections
    if (listen(serverSocket, SOMAXCONN) == SOCKET_ERROR) {
        cout << "Listen failed: " << WSAGetLastError() << endl;
        closesocket(serverSocket);
        WSACleanup();
        exit(1);
    }
    cout << "Server is listening for incoming connections..." << endl;



    // Step 5: Accept a connection
    sockaddr_in clientAddr;
    int clientAddrSize = sizeof(clientAddr);
    vector<SOCKET> clients;

    while (1) {
        SOCKET clientSocket = accept(serverSocket, (sockaddr*)&clientAddr, &clientAddrSize);
        if (clientSocket == INVALID_SOCKET) {
            cout << "Accept failed: " << WSAGetLastError() << endl;
            continue; // Just continue to next iteration, don't shutdown server
        }
        { // Use a block to scope the lock
            lock_guard<mutex> guard(clients_mutex); // <<< FIX: Lock the mutex
            clients.push_back(clientSocket);
        }
        // FIX: Corrected thread constructor syntax and detach typo
        thread t(clientHandler, clientSocket, std::ref(clients));
        t.detach();
        //detatch here and join in client
        //because here detach is in while loop it will keep running
        //client mein for loop ni h
    }
    //ek client send msssage to server,server send to everyone joh joh client uss server se connected hai
    //thread t1 intteract server with that client, and main thread busy acceping connection from other clients
   //jabh send krna ho clients vector tells kis kisko send krna h, we will pass vector as reefrence in thread:
    //why reference: while client is interacting with server agar uss beech aur client aagye and gets filled in vector 
    //so it will reflect in original vector

    closesocket(serverSocket);
    WSACleanup();
    cout << "Server shut down." << endl;

    return 0;
}