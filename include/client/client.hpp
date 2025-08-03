#ifndef CLIENT_HPP
#define CLIENT_HPP
#include <iostream>
#include <string>
#include <vector>
#include <future>
#include <atomic>
#include <csignal>
#include <chrono>
#include <locale>

#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>


#pragma comment (lib, "ws2_32.lib")
/**
 * @class Client
 * @brief �����, �������������� ������ ��� �������� ��������������.
 * ������������ �������������� ���������������, ����������� ����� ���������.
 */
class Client {
public:
	/**
	 * @brief ��� ������� ��������� ������ ��� ��������� ���������� ���������.
	 */
	using MessageCallback = std::function<void(const std::string&)>;
	/**
	 * @brief ����������� �������
	 * @param serverAddr IP-����� ������� (������ "127.0.0.1")
	 * @param port ���� ������� (1-65535)
	 */
	Client(const std::string& serverAddr, uint16_t port);
	/**
	 * @brief ���������� �������
	 * @note ������������� ����������� �� �������
	 */
	~Client();
	/**
	 * @brief ����������� � �������
	 * @return true - ����������� �������, false - ������
	 * @note ������������� �������� �� ����� (10 ������)
	 */
	bool connectToServer();
	/**
	 * @brief ���������� �� �������
	 * @note ������������� ����� ������ ���������
	 */
	void disconnect();
	/**
	 * @brief �������� ���������
	 * @param message ����� ���������
	 * @return true - ���������� �������, false - ������
	 * @note ��� ������ ������������� �������� ���������������� (���� ������� autoReconnect)
	 */
	bool sendMessage(const std::string& message);
	/**
	 * @brief �������� ��������� �����������
	 * @return true - ������ ���������, false - ��������
	 */
	bool isConnected() const { return m_connected; };
	/**
	 * @brief ��������� ������� ������ ��������� �� �������.
	 * ���� ������ ���������, ������� � ��������� ����� ����� ��� ������
	 * ������ `receiveMessages`.
	 */
	void startReceiving();
	/**
	 * @brief ������������� ������� ������ ��������� �� �������.
	 * ������������� ���� `m_receiving` � false � ������� ����������
	 * ������ `m_receiveThread`.
	 */
	void stopReceiving();
	/**
	 * @brief ������� ���������������
	 * @return true - ��������������� �������, false - ��������� �������
	 * @note ���������� ��������� m_reconnectDelayMs � m_maxReconnectAttempts
	 */
	bool tryReconnect();
private:
	/**
	 * @brief ��������� ������� ��� ������ ���������
	 * @details ���������� ����� 4096 ����, ������������ ��������
	 */
	void receiveMessages();
	std::string m_username; // ��� ������������ �������.
	SOCKET m_socket = INVALID_SOCKET; // ���������� ������ �������.
	std::string m_serverAddr; // ����� �������.
	uint16_t m_port; // ���� �������.
	std::atomic <bool> m_connected{ false }; // ����, �����������, ����������� �� ���������� � ��������.
	std::atomic <bool> m_receiving{ false }; // ����, �����������, ���� �� ������� ������ ���������.
	std::thread m_receiveThread; // ����� ��� ������������ ������ ���������.
	std::atomic<bool> m_autoReconnect{ false }; // ����, �����������, ������� �� ������������� ����������������.
	int m_reconnectDelayMs = 10000; // �������� ����� ��������� ��������������� (� �������������).
	DWORD timeout = 100000; // ����-��� ��� �������� � �������� (� �������������).
	int m_maxReconnectAttempts = 3; // ������������ ���������� ������� ��������������� ���������������.
};
#endif