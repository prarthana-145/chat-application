#include <iostream>
#include <winsock2.h> // Include Windows socket library
#pragma comment(lib, "ws2_32.lib") // Link winsock library
#include<WS2tcpip.h>
#include<thread>
#include <string>
#include <atomic>
using namespace std;
//client has to both send and receive msg
//2 threads:1 will send other will receive

// Global flag to signal when to stop
atomic<bool> shouldStop(false);

void initializeWinsock() {
    WSADATA wsaData;
    int result = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (result != 0) {
        cout << "WSAStartup failed: " << result << endl;
        exit(1);
    }
    cout << "Winsock initialized successfully!" << endl;
}

void SendMsg(SOCKET clientSocket) {
    cout << "enter your chat name : " << endl;
    string name;
    getline(cin, name);
    cout << "Start your conversation" << endl;
    cout << "enter quit to end conversation" << endl;
    string message;
    while (1) {
        getline(cin, message);
        string msg = name + " : " + message;
        int bytessent = send(clientSocket, msg.c_str(), msg.length(), 0);
        if (bytessent == SOCKET_ERROR) {
            cout << "error sending message" << endl;
            break;
        }
        if (message == "quit")
        {
            cout << "stopping the application" << endl;
            shouldStop = true; // Signal receiver thread to stop
            break;
        }
    }
}

void ReceiveMsg(SOCKET clientSocket) {
    char buffer[4096];
    int recvlength;
    string msg = "";
    while (1) {
        recvlength = recv(clientSocket, buffer, sizeof(buffer), 0);
        if (recvlength <= 0) {
            cout << "disconnected from the server" << endl;
            break;
        }
        else {
            msg = string(buffer, recvlength);
            cout << msg << endl;
        }
        // Check if we should stop (quit was entered in sender thread)
        if (shouldStop) {
            break;
        }
    }
}

int main() {
    // Step 1: Initialize Winsock library
    initializeWinsock();
    // Step 2: Create a socket
    string serveraddr = "127.0.0.1";
    int port = 12345;
    SOCKET clientSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (clientSocket == INVALID_SOCKET) {
        cout << "Socket creation failed: " << WSAGetLastError() << endl;
        WSACleanup();
        exit(1);
    }
    cout << "Socket created successfully!" << endl;
    // Step 3: Define the server address to connect to
    sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(12345); // Port should match the server
    //serverAddr.sin_addr.s_addr = inet_addr("127.0.0.1"); // Localhost (adjust for actual server IP)
    inet_pton(AF_INET, serveraddr.c_str(), &(serverAddr.sin_addr));
    // Step 4: Connect to the server
    int result = connect(clientSocket, (sockaddr*)&serverAddr, sizeof(serverAddr));
    if (result == SOCKET_ERROR) {
        cout << "Connection to server failed: " << WSAGetLastError() << endl;
        closesocket(clientSocket);
        WSACleanup();
        exit(1);
    }
    cout << "Connected to the server!" << endl;
    thread senderthread(SendMsg, clientSocket);
    thread receiverthread(ReceiveMsg, clientSocket);
    senderthread.join();
    // Close socket after sender finishes to signal receiver
    closesocket(clientSocket);
    receiverthread.join();
    WSACleanup();
    return 0;
}