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
 * @brief Класс, представляющий клиент для сетевого взаимодействия.
 * Поддерживает автоматическое переподключение, асинхронный прием сообщений.
 */
class Client {
public:
	/**
	 * @brief Тип функции обратного вызова для обработки полученных сообщений.
	 */
	using MessageCallback = std::function<void(const std::string&)>;
	/**
	 * @brief Конструктор клиента
	 * @param serverAddr IP-адрес сервера (формат "127.0.0.1")
	 * @param port Порт сервера (1-65535)
	 */
	Client(const std::string& serverAddr, uint16_t port);
	/**
	 * @brief Деструктор клиента
	 * @note Автоматически отключается от сервера
	 */
	~Client();
	/**
	 * @brief Подключение к серверу
	 * @return true - подключение успешно, false - ошибка
	 * @note Устанавливает таймауты на сокет (10 секунд)
	 */
	bool connectToServer();
	/**
	 * @brief Отключение от сервера
	 * @note Останавливает поток приема сообщений
	 */
	void disconnect();
	/**
	 * @brief Отправка сообщения
	 * @param message Текст сообщения
	 * @return true - отправлено успешно, false - ошибка
	 * @note При ошибке автоматически пытается переподключиться (если включен autoReconnect)
	 */
	bool sendMessage(const std::string& message);
	/**
	 * @brief Проверка состояния подключения
	 * @return true - клиент подключен, false - отключен
	 */
	bool isConnected() const { return m_connected; };
	/**
	 * @brief Запускает процесс приема сообщений от сервера.
	 * Если клиент подключен, создает и запускает новый поток для работы
	 * метода `receiveMessages`.
	 */
	void startReceiving();
	/**
	 * @brief Останавливает процесс приема сообщений от сервера.
	 * Устанавливает флаг `m_receiving` в false и ожидает завершения
	 * потока `m_receiveThread`.
	 */
	void stopReceiving();
	/**
	 * @brief Попытка переподключения
	 * @return true - переподключение успешно, false - превышены попытки
	 * @note Использует настройки m_reconnectDelayMs и m_maxReconnectAttempts
	 */
	bool tryReconnect();
private:
	/**
	 * @brief Потоковая функция для приема сообщений
	 * @details Использует буфер 4096 байт, обрабатывает таймауты
	 */
	void receiveMessages();
	std::string m_username; // Имя пользователя клиента.
	SOCKET m_socket = INVALID_SOCKET; // Дескриптор сокета клиента.
	std::string m_serverAddr; // Адрес сервера.
	uint16_t m_port; // Порт сервера.
	std::atomic <bool> m_connected{ false }; // Флаг, указывающий, установлено ли соединение с сервером.
	std::atomic <bool> m_receiving{ false }; // Флаг, указывающий, идет ли процесс приема сообщений.
	std::thread m_receiveThread; // Поток для асинхронного приема сообщений.
	std::atomic<bool> m_autoReconnect{ false }; // Флаг, указывающий, следует ли автоматически переподключаться.
	int m_reconnectDelayMs = 10000; // Задержка между попытками переподключения (в миллисекундах).
	DWORD timeout = 100000; // Тайм-аут для операций с сокетами (в миллисекундах).
	int m_maxReconnectAttempts = 3; // Максимальное количество попыток автоматического переподключения.
};
#endif