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
 * @class Server управляет процессом принятия входящих соединений,
 * @brief обрабатывает клиентов в отдельных потоках и обеспечивает
 * базовую функциональность для клинт-серверного приложения.
 * Сервер поддерживает асинхронную обработку клиентов с ограничением
 * максимального количества одновременных подключений.
 */

class Server {
private:
    DWORD timeout = 100000; // Таймаут сокета в миллисекундах
    DWORD recvTimeout = 10000; // Таймаут recv в миллисекундах
    const size_t MAX_MESSAGE_SIZE = 4096; // Максимальный размер сообщения
    static constexpr int MAX_CLIENTS = 100; // Максимальное число клиентов
    std::atomic<int> m_activeClients{ 0 }; // Атомарный счетчик активных клиентов
    SOCKET m_serverSocket = INVALID_SOCKET; // Сокет сервера
    uint16_t m_port; // Порт, на котором будет работать сервер
    std::atomic<bool> m_running{ false }; // Атомарный флаг, указывающий, работает ли сервер
    std::mutex m_futuresMutex;    // Мьютекс для защиты доступа к вектору m_clientFutures
    std::mutex m_clientsMutex;
    std::vector<std::future<void>> m_clientFutures; // Вектор для хранения futures, представляющих потоки обработки клиентов
    /**
    * @brief Обрабатывает соединение с конкретным клиентом.
    * Этот метод вызывается в отдельном потоке для каждого нового клиента.
    * Он отвечает за прием, обработку и отправку данных клиенту.
    * @param clientSocket Дескриптор сокета клиента.
    */
    void handleClient(SOCKET clientSocket);
    /**
    * @brief Освобождает все ресурсы, связанные с сервером.
    * Закрывает серверный сокет и выполняет другую необходимую очистку.
    */
    void cleanup();
    /**
     * @brief Основной цикл сервера для приема входящих соединений.
     *
     * Этот метод работает в отдельном потоке, постоянно ожидая новых
     * подключений клиентов и запуская для каждого новый поток обработки.
     */
    void acceptConnections();
public:
    /**
     * @brief Конструктор сервера
     * @param port Порт для прослушивания (1-65535)
     */
    explicit Server(uint16_t port = 8080);
    /**
     * @brief Деструктор сервера
     * @note Автоматически останавливает сервер при уничтожении
     */
    ~Server();
    /**
   * @brief Возвращает количество активных клиентов
   * @return Текущее число подключенных клиентов
   */
    int getActiveClients() const { return m_activeClients; }
    /**
    * @brief Запускает сервер
    * @return true - сервер запущен, false - ошибка запуска
    * @note Инициализирует Winsock, создает сокет и начинает прослушивание
    */
    bool start();
    /**
     * @brief Останавливает сервер
     * @note Закрывает все соединения и освобождает ресурсы
     */
    void stop();
    /**
    * @brief Проверяет работает ли сервер
    * @return true - сервер активен, false - остановлен
    */
    bool isRunning() const { return m_running; }
};
/**
 * @class InputValidator
 * @brief Валидатор входящих сообщений
 */
class InputValidator {
public:
    /**
    * @brief Проверяет корректность сообщения
    * @param message Сообщение для проверки
    * @return true - сообщение валидно, false - содержит ошибки
    *
    * Валидация включает проверки:
    * - Не пустое сообщение
    * - Длина ? MAX_MESSAGE_LENGTH
    * - Отсутствие запрещенных символов
    */
    static bool validateMessage(const std::string& message) {
        return !message.empty() &&
            message.length() <= MAX_MESSAGE_LENGTH &&
            !containsInvalidChars(message);
    }
private:
    static constexpr size_t MAX_MESSAGE_LENGTH = 1024;// Максимальная допустимая длина сообщения.
    /**
     * @brief Проверяет, содержит ли сообщение недопустимые символы.
     *
     * Недопустимыми считаются символы с ASCII кодами от 0 до 8,
     * от 11 до 12, от 14 до 31, а также символ DEL (127).
     *
     * @param message Строка для проверки.
     * @return true, если найдены недопустимые символы, false в противном случае.
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