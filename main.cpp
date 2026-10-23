#define WIN32_LEAN_AND_MEAN
#define LOCALTEST
#include <winsock2.h>
#include <ws2tcpip.h>
#include <iostream>
#include <string>
#include <cstring>

#pragma comment(lib, "ws2_32.lib") // 使用 MSVC 时可自动链接（CMake 中也已链接）

static void printWinsockError(const char* msg) {
    int err = WSAGetLastError();
    std::cerr << msg << " (error code: " << err << ")" << std::endl;
}

int main(int argc, char* argv[]) {
    SetConsoleOutputCP(CP_UTF8);   // 控制台输出用 UTF-8
    SetConsoleCP(CP_UTF8);         // 控制台输入也用 UTF-8（如需输入中文）
    std::string host = "47.93.189.173";
#ifdef LOCALTEST
    host = "127.0.0.1";
#endif
    std::string port = "8080";
    WSADATA wsaData;
    int iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (iResult != 0) {
        std::cerr << "WSAStartup failed: " << iResult << std::endl;
        return 1;
    }

    // 2. 使用 getaddrinfo 解析地址
    addrinfo hints{};
    hints.ai_family   = AF_UNSPEC;   // IPv4 或 IPv6
    hints.ai_socktype = SOCK_STREAM; // TCP
    hints.ai_protocol = IPPROTO_TCP;

    addrinfo* result = nullptr;
    iResult = getaddrinfo(host.c_str(), port.c_str(), &hints, &result);
    if (iResult != 0) {
        std::cerr << "getaddrinfo failed: " << iResult << std::endl;
        WSACleanup();
        return 1;
    }

    // 3. 尝试连接
    SOCKET connectSocket = INVALID_SOCKET;
    for (addrinfo* ptr = result; ptr != nullptr; ptr = ptr->ai_next) {
        connectSocket = socket(ptr->ai_family, ptr->ai_socktype, ptr->ai_protocol);
        if (connectSocket == INVALID_SOCKET) {
            printWinsockError("socket failed");
            continue;
        }

        iResult = connect(connectSocket, ptr->ai_addr, (int)ptr->ai_addrlen);
        if (iResult == SOCKET_ERROR) {
            int err = WSAGetLastError();
            std::cerr << "connect failed, error = " << err << " ("
                      << (err == WSAETIMEDOUT   ? "TIMEOUT" :
                          err == WSAECONNREFUSED? "CONNECTION REFUSED" :
                          err == WSAEHOSTUNREACH? "HOST UNREACHABLE" :
                          err == WSAENETUNREACH ? "NETWORK UNREACHABLE" :
                          err == WSAETIMEDOUT   ? "TIMEOUT" : "OTHER")
                      << ")\n";
            closesocket(connectSocket);
            connectSocket = INVALID_SOCKET;
            continue;
        }
        break; // 连接成功
    }

    freeaddrinfo(result);

    if (connectSocket == INVALID_SOCKET) {
        std::cerr << "Unable to connect to server " << host << ":" << port << std::endl;
        WSACleanup();
        return 1;
    }

    std::cout << "Connected to " << host << ":" << port << std::endl;
    std::cout << "输入消息发送（输入 'quit' 退出）:\n";

    // 4. 发送/接收循环
    std::string line;
    const int DEFAULT_BUFLEN = 512;
    char recvbuf[DEFAULT_BUFLEN];

    while (true) {
        std::cout << "> ";
        if (!std::getline(std::cin, line)) break;
        if (line == "quit" || line == "exit") break;

        line += "\n";
        int sendResult = send(connectSocket, line.c_str(), (int)line.size(), 0);
        if (sendResult == SOCKET_ERROR) {
            printWinsockError("send failed");
            break;
        }
        std::cout << "Bytes Sent: " << sendResult << std::endl;

        // 接收回显
        int recvResult = recv(connectSocket, recvbuf, DEFAULT_BUFLEN - 1, 0);
        if (recvResult > 0) {
            recvbuf[recvResult] = '\0';
            std::cout << "Bytes received: " << recvResult
                      << "  Data: " << recvbuf << std::endl;
        } else if (recvResult == 0) {
            std::cout << "Connection closed by server." << std::endl;
            break;
        } else {
            printWinsockError("recv failed");
            break;
        }
    }

    // 5. 关闭连接
    shutdown(connectSocket, SD_SEND);
    closesocket(connectSocket);
    WSACleanup();

    std::cout << "Client exiting." << std::endl;
    return 0;
}