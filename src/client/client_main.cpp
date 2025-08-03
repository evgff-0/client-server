#include "../include/client/client.hpp"

int main() {
    setlocale(LC_ALL, "Russian");
    SetConsoleCP(1251);
    SetConsoleOutputCP(1251);

    Client client("127.0.0.1", 8080);

    if (!client.connectToServer()) {
        std::cerr << "Failed to connect to server\n";
        return 1;
    }

    std::cout << "Connected to server. Type messages to send (type 'exit' to quit):\n";
    client.startReceiving();

    std::string message;
    while (true) {
        std::getline(std::cin, message);

        if (message == "exit") {
            break;
        }

        if (!client.sendMessage(message + "\n")) {
            std::cerr << "Message send failed\n";
            break;
        }
    }

    client.disconnect();
    return 0;
}
