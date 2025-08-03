#include "../include/client/client.hpp"

/**
 * @brief Инициализирует Winsock и параметры подключения
 * @param serverAddr Адрес сервера (копируется)
 * @param port Порт сервера (проверяется в connectToServer())
 */
Client::Client(const std::string& serverAddr, uint16_t port)
	: m_serverAddr(serverAddr), m_port(port) {
	WSADATA wsaData;
	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
		std::cerr << "WSAStartup failed" << std::endl;
	}
}

Client::~Client() {
	disconnect();
}

/**
 * @brief Устанавливает соединение с сервером
 * @note Устанавливает таймауты SO_RCVTIMEO/SO_SNDTIMEO (10 сек)
 * @return true если подключение успешно
 */
bool Client::connectToServer() {
	if (m_connected) return true;
	// Создание сокета
	m_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (m_socket == INVALID_SOCKET) {
		std::cerr << "Socket creation failed: " << WSAGetLastError() << std::endl;
		return false;
	}
	// Настройка таймаутов
	setsockopt(m_socket, SOL_SOCKET, SO_RCVTIMEO, (char*)&timeout, sizeof(timeout));
	setsockopt(m_socket, SOL_SOCKET, SO_SNDTIMEO, (char*)&timeout, sizeof(timeout));
	// Параметры подключения
	sockaddr_in serverAddr{};
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_port = htons(m_port);
	inet_pton(AF_INET, m_serverAddr.c_str(), &serverAddr.sin_addr);
	if (connect(m_socket, (sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
		std::cerr << "Connection Failed: " << WSAGetLastError() << std::endl;
		closesocket(m_socket);
		m_socket = INVALID_SOCKET;
		return false;
	}
	m_connected = true;
	return true;
}

/**
 * @brief Циклически пытается переподключиться при разрыве соединения
 * @details Использует настройки m_reconnectDelayMs и m_maxReconnectAttempts
 */
bool Client::tryReconnect() {
	if (!m_autoReconnect) return false;

	int attempts = 0;
	while (attempts < m_maxReconnectAttempts && !m_connected) {
		std::cerr << "Reconnecting (attempt " << attempts + 1 << ")...\n";
		std::this_thread::sleep_for(std::chrono::milliseconds(m_reconnectDelayMs));
		if (m_socket != INVALID_SOCKET) {
			closesocket(m_socket);
			m_socket = INVALID_SOCKET;
		}
		if (connectToServer()) {
			std::cout << "Reconnected successfully!\n";
			if (m_receiving) startReceiving();
			return true;
		}
		attempts++;
	}
	return false;
}

/**
 * @brief Корректно останавливает прием данных и закрывает сокет
 */
void Client::disconnect() {
	if (!m_connected) return;
	stopReceiving();
	closesocket(m_socket);
	m_socket = INVALID_SOCKET;
	m_connected = false;
}

/**
 * @brief Отправляет строковое сообщение серверу
 * @warning При пустом сообщении только логирует предупреждение (возвращает true)
 */
bool Client::sendMessage(const std::string& message) {
	if (!m_connected) {
		if (!tryReconnect()) {
			std::cerr << "Cannot send - not connected to server\n";
			return false;
		}
	}
	if (message.empty()) {
		std::cerr << "Warning: Attempt to send empty message\n";
		return true;
	}
	int result = send(m_socket, message.c_str(), static_cast<int>(message.size()), 0);
	if (result == SOCKET_ERROR) {
		std::cerr << "Send failed: " << WSAGetLastError() << "\n";
		disconnect();
		return false;
	}
	if (result != static_cast<int>(message.size())) {
		std::cerr << "Warning: Partial message sent (" << result << "/" << message.size() << " bytes)\n";
	}
	return true;
}

/**
 * @brief Запускает асинхронный прием сообщений в отдельном потоке
 */
void Client::startReceiving() {
	if (!m_connected || m_receiving) return;

	m_receiving = true;
	m_receiveThread = std::thread(&Client::receiveMessages, this);
}

/**
 * @brief Безопасно останавливает поток приема сообщений
 */
void Client::stopReceiving() {
	if (!m_receiving) return;

	m_receiving = false;
	if (m_receiveThread.joinable()) {
		m_receiveThread.join();
	}
}
/**
 * @brief Основной цикл приема данных
 * @details Обрабатывает:
 * - Разрыв соединения
 * - Таймауты
 * - Ошибки сокета
 */
void Client::receiveMessages() {
	constexpr size_t BUFFER_SIZE = 4096;
	std::vector<char> buffer(BUFFER_SIZE);
	while (m_receiving) {
		if (!m_connected) {
			if (!tryReconnect()) break; // Прекращаем при отсутствии переподключения
			continue;
		}
		int bytesReceived = recv(m_socket, buffer.data(), buffer.size() - 1, 0);

		// Обработка результатов recv
		if (bytesReceived > 0) {
			const size_t safe_size = bytesReceived < buffer.size() ? bytesReceived : buffer.size() - 1;
			buffer[safe_size] = '\0';
			std::cout << "Received: " << buffer.data() << "\n";
		}
		else if (bytesReceived == 0) {
			std::cout << "Server disconnected\n";
			disconnect();
			break;
		}
		else {
			if (WSAGetLastError() != WSAETIMEDOUT) {
				std::cerr << "Receive error: " << WSAGetLastError() << "\n";
				disconnect();
				break;
			}
		}
	}
}