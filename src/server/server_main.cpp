#include "../include/server/server.hpp"

std::atomic<bool> g_running{ true };

void signalHandler(int) {
    g_running = false;
}

int main() {
    setlocale(LC_ALL, "Russian");
    SetConsoleCP(1251);
    SetConsoleOutputCP(1251);
    std::signal(SIGINT, signalHandler);

    Server server(8080);
    if (!server.start()) {
        return 1;
    }

    std::cout << "Server running. Press Ctrl+C to stop.\n";
    while (g_running) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    server.stop();
    return 0;
}