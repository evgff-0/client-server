#include "../include/server/server.hpp"

/**
 * @brief �������������� ������ � ��������� ������
 * @param port ���� ��� ������������� (1-65535)
 * @throw std::invalid_argument ��� ������������ �������� �����
 */
Server::Server(uint16_t port) : m_port(port) {
    if (port == 0 || port > 65535) {
        throw std::invalid_argument("Port must be between 1 and 65535");
    }
}

/**
 * @brief ��������� ������������� ������
 */
Server::~Server() {
    stop();
}

/**
 * @brief ��������� ��������� ����� � ����� ��������� �����������
 * @return true ��� �������� �������, false ��� ������
**/
bool Server::start() {
    if (m_running) return true;

    // ������������� Winsock
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        std::cerr << "WSAStartup failed\n";
        return false;
    }

    // �������� ������
    m_serverSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (m_serverSocket == INVALID_SOCKET) {
        std::cerr << "Socket creation failed\n";
        return false;
    }

    // ��������� ������ �������
    sockaddr_in serverAddr{};
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port = htons(m_port);

    if (bind(m_serverSocket, (sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
        std::cerr << "Bind failed\n";
        cleanup();
        return false;
    }

    if (listen(m_serverSocket, SOMAXCONN) == SOCKET_ERROR) {
        std::cerr << "Listen failed\n";
        cleanup();
        return false;
    }

    m_running = true;
    std::thread(&Server::acceptConnections, this).detach();
    return true;
}

/**
 * @brief ������������� ������ � ����������� �������
 **/
void Server::stop() {
    m_running = false;

    // ���������� ���������� futures
    std::vector<std::future<void>> futures;
    {
        std::lock_guard<std::mutex> lock(m_futuresMutex);
        futures = std::move(m_clientFutures);
    }

    // �������� ���������� ���� ���������� �������
    for (auto& future : futures) {
        if (future.valid()) future.wait();
    }
    cleanup();
}

/**
 * @brief �������� ���� �������� �����������
 * @details ���������� select() ��� ������������� �������� ������
 */
void Server::acceptConnections() {
    while (m_running) {
        fd_set readSet;
        FD_ZERO(&readSet);
        FD_SET(m_serverSocket, &readSet);
        timeval timeout{ 0, 100000 };

        if (select(0, &readSet, nullptr, nullptr, &timeout) > 0) {
            if (m_activeClients >= MAX_CLIENTS) {
                // ���������� ����������� ��� ����������
                std::cerr << "Client rejected: server is full!\n";
                SOCKET tempSocket = accept(m_serverSocket, nullptr, nullptr);
                if (tempSocket != INVALID_SOCKET) {
                    const char* msg = "Server is busy. Try again later.\n";
                    send(tempSocket, msg, strlen(msg), 0);
                    closesocket(tempSocket);
                }
                continue;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            SOCKET clientSocket = accept(m_serverSocket, nullptr, nullptr);
            if (clientSocket == INVALID_SOCKET) {
                std::cerr << "Accept failed: " << WSAGetLastError() << "\n";
                continue;
            }
            else {
                // ��������� � ������ ����������� �������
                setsockopt(clientSocket, SOL_SOCKET, SO_RCVTIMEO, (char*)&recvTimeout, sizeof(recvTimeout));
                {
                    std::lock_guard<std::mutex> lock(m_clientsMutex);
                    m_activeClients++;
                }
                {
                    std::lock_guard<std::mutex> lock(m_futuresMutex);
                    m_clientFutures.emplace_back(
                        std::async(std::launch::async, [this, clientSocket]() {
                            this->handleClient(clientSocket);
                            std::lock_guard<std::mutex> lock(m_clientsMutex);
                            m_activeClients--;
                            })
                    );

                }
            }
        }
        // ������������� ������� ����������� futures
        {
            std::lock_guard<std::mutex> lock(m_futuresMutex);
            m_clientFutures.erase(
                std::remove_if(m_clientFutures.begin(), m_clientFutures.end(),
                    [](auto& future) {
                        return !future.valid() ||
                            (future.wait_for(std::chrono::seconds(0)) == std::future_status::ready); }),
                m_clientFutures.end()
            );
        }
    }
}

/**
 * @brief ������������ ���������� � ��������
 * @param clientSocket ����� ������������� �������
 **/
void Server::handleClient(SOCKET clientSocket) {
    auto logDisconnect = [clientSocket](const std::string& reason) {
        std::cout << "Client [" << clientSocket << "] disconnected. Reason: " << reason << "\n";
        };

    auto cleanup = [this, clientSocket, &logDisconnect]() {
        closesocket(clientSocket);
        std::lock_guard<std::mutex> lock(m_clientsMutex);
        m_activeClients--;
        };

    try {
        // ��������� ������
        setsockopt(clientSocket, SOL_SOCKET, SO_RCVTIMEO, (char*)&timeout, sizeof(timeout));
        std::cout << "New client connected. Socket: " << clientSocket << std::endl;

        std::vector<char> buffer(MAX_MESSAGE_SIZE);
        int timeoutCount = 0;

        while (m_running) {
            int bytesReceived = recv(clientSocket, buffer.data(), buffer.size(), 0);

            if (bytesReceived > 0) {
                timeoutCount = 0;

                // �������� �� ������������ ������
                if (bytesReceived >= MAX_MESSAGE_SIZE) {
                    logDisconnect("buffer overflow protection");
                    const char* msg = "Error: Message too large\n";
                    send(clientSocket, msg, strlen(msg), 0);
                    cleanup();
                    return;
                }

                // ��������� ���������
                std::string message(buffer.data(), bytesReceived);
                if (!InputValidator::validateMessage(message)) {
                    logDisconnect("invalid message format");
                    const char* errorMsg = "Error: Invalid message format\n";
                    send(clientSocket, errorMsg, strlen(errorMsg), 0);
                    cleanup();
                    return;
                }

                std::cout << "Client [" << clientSocket << "]: " << message << std::endl;
                send(clientSocket, message.c_str(), message.size(), 0);
            }
            else if (bytesReceived == 0) {
                logDisconnect("graceful disconnect");
                cleanup();
                return;
            }
            else {
                // ��������� ������
                int error = WSAGetLastError();
                if (error == WSAETIMEDOUT) {
                    if (++timeoutCount > 3) {
                        logDisconnect("timeout");
                        cleanup();
                        return;
                    }
                    continue;
                }

                logDisconnect("socket error: " + std::to_string(error));
                cleanup();
                return;
            }
        }
    }
    catch (...) {
        std::cerr << "Exception in client handler for socket " << clientSocket << std::endl;
        cleanup();
    }
}

/**
 * @brief ����������� ������� �������
 */
void Server::cleanup() {
    if (m_serverSocket != INVALID_SOCKET) {
        closesocket(m_serverSocket);
        m_serverSocket = INVALID_SOCKET;
    }
    WSACleanup();
}