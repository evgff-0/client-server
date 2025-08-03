#ifndef SERVER_HPP
#define SERVER_HPP

#include <iostream>
#include <string>
#include <vector>
#include <future>
#include <atomic>
#include <csignal>
#include <regex>
#include <locale>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>

#pragma comment (lib, "ws2_32.lib")
/**
 * @class Server ��������� ��������� �������� �������� ����������,
 * @brief ������������ �������� � ��������� ������� � ������������
 * ������� ���������������� ��� �����-���������� ����������.
 * ������ ������������ ����������� ��������� �������� � ������������
 * ������������� ���������� ������������� �����������.
 */

class Server {
private:
    DWORD timeout = 100000; // ������� ������ � �������������
    DWORD recvTimeout = 10000; // ������� recv � �������������
    const size_t MAX_MESSAGE_SIZE = 4096; // ������������ ������ ���������
    static constexpr int MAX_CLIENTS = 100; // ������������ ����� ��������
    std::atomic<int> m_activeClients{ 0 }; // ��������� ������� �������� ��������
    SOCKET m_serverSocket = INVALID_SOCKET; // ����� �������
    uint16_t m_port; // ����, �� ������� ����� �������� ������
    std::atomic<bool> m_running{ false }; // ��������� ����, �����������, �������� �� ������
    std::mutex m_futuresMutex;    // ������� ��� ������ ������� � ������� m_clientFutures
    std::mutex m_clientsMutex;
    std::vector<std::future<void>> m_clientFutures; // ������ ��� �������� futures, �������������� ������ ��������� ��������
    /**
    * @brief ������������ ���������� � ���������� ��������.
    * ���� ����� ���������� � ��������� ������ ��� ������� ������ �������.
    * �� �������� �� �����, ��������� � �������� ������ �������.
    * @param clientSocket ���������� ������ �������.
    */
    void handleClient(SOCKET clientSocket);
    /**
    * @brief ����������� ��� �������, ��������� � ��������.
    * ��������� ��������� ����� � ��������� ������ ����������� �������.
    */
    void cleanup();
    /**
     * @brief �������� ���� ������� ��� ������ �������� ����������.
     *
     * ���� ����� �������� � ��������� ������, ��������� ������ �����
     * ����������� �������� � �������� ��� ������� ����� ����� ���������.
     */
    void acceptConnections();
public:
    /**
     * @brief ����������� �������
     * @param port ���� ��� ������������� (1-65535)
     */
    explicit Server(uint16_t port = 8080);
    /**
     * @brief ���������� �������
     * @note ������������� ������������� ������ ��� �����������
     */
    ~Server();
    /**
   * @brief ���������� ���������� �������� ��������
   * @return ������� ����� ������������ ��������
   */
    int getActiveClients() const { return m_activeClients; }
    /**
    * @brief ��������� ������
    * @return true - ������ �������, false - ������ �������
    * @note �������������� Winsock, ������� ����� � �������� �������������
    */
    bool start();
    /**
     * @brief ������������� ������
     * @note ��������� ��� ���������� � ����������� �������
     */
    void stop();
    /**
    * @brief ��������� �������� �� ������
    * @return true - ������ �������, false - ����������
    */
    bool isRunning() const { return m_running; }
};
/**
 * @class InputValidator
 * @brief ��������� �������� ���������
 */
class InputValidator {
public:
    /**
    * @brief ��������� ������������ ���������
    * @param message ��������� ��� ��������
    * @return true - ��������� �������, false - �������� ������
    *
    * ��������� �������� ��������:
    * - �� ������ ���������
    * - ����� ? MAX_MESSAGE_LENGTH
    * - ���������� ����������� ��������
    */
    static bool validateMessage(const std::string& message) {
        return !message.empty() &&
            message.length() <= MAX_MESSAGE_LENGTH &&
            !containsInvalidChars(message);
    }
private:
    static constexpr size_t MAX_MESSAGE_LENGTH = 1024;// ������������ ���������� ����� ���������.
    /**
     * @brief ���������, �������� �� ��������� ������������ �������.
     *
     * ������������� ��������� ������� � ASCII ������ �� 0 �� 8,
     * �� 11 �� 12, �� 14 �� 31, � ����� ������ DEL (127).
     *
     * @param message ������ ��� ��������.
     * @return true, ���� ������� ������������ �������, false � ��������� ������.
     */
    static bool containsInvalidChars(const std::string& message) {
        for (unsigned char c : message) {
            if (c < 32 && c != 0x09 && c != 0x0A) return true;
            if (c == 0x7F) return true;
        }
        return false;
    }
};
#endif