#include "Client.h"
#include <arpa/inet.h>
#include <atomic>
#include <iostream>
#include <netdb.h>
#include <netinet/in.h>
#include <stdexcept>
#include <string>
#include <sys/socket.h>
#include <unistd.h>

#if defined(_WIN32) || defined(_WIN64)
#include <io.h>
#define IS_A_TTY _isatty
#define FILENO _fileno
#else
#include <unistd.h>
#define IS_A_TTY isatty
#define FILENO fileno
#endif

#define ANSI_RED_TEXT "\033[31m"

#define ANSI_RESET "\033[0m"

std::string ClientConfig::ServerStr() {
    return this->server + ":" + std::to_string(this->port);
}

Client::Client(ClientConfig config, const std::atomic<bool> &stop)
    : config_(config), stop_(stop) {};

void Client::Operate() {
    if (config_.is_repl) {
        std::cout << "Welcome to the repl!  Enter \"help\" for assistance.\n";
        while (!stop_) {
            auto cmd_str = AwaitCommand();
            if (!cmd_str)
                continue;
            SendCommand(cmd_str.value());
        }
    } else {
        SendCommand(config_.command);
    }
}

void Client::SendCommand(const std::string command) {
    addrinfo hints{};
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    addrinfo *res = nullptr;
    if (getaddrinfo(config_.server.c_str(),
                    std::to_string(config_.port).c_str(), &hints, &res) != 0 ||
        res == nullptr)
        throw std::runtime_error("Failed to resolve server: " +
                                 config_.ServerStr());

    int server_socket =
        socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    if (server_socket == -1) {
        freeaddrinfo(res);
        throw std::runtime_error("Failed to create socket.");
    }

    int rc = connect(server_socket, res->ai_addr, res->ai_addrlen);
    freeaddrinfo(res);
    if (rc) {
        close(server_socket);
        throw std::runtime_error("Failed to connect to server: " +
                                 config_.ServerStr());
    }

    std::string message = command + "\n";

    if (send(server_socket, message.data(), message.size(), MSG_NOSIGNAL) < 0) {
        close(server_socket);
        throw std::runtime_error("Message failed to send");
    }

    std::string response;
    char buffer[1024];

    while (!response.ends_with('\n') && response.size() < max_response_len) {
        ssize_t bytes_received = recv(server_socket, buffer, sizeof(buffer), 0);
        if (bytes_received == 0) {
            close(server_socket);
            throw std::runtime_error("Server closed the connection");
        }

        if (bytes_received < 0) {
            close(server_socket);
            throw std::runtime_error("Unexpected recv failure");
        }

        response.append(buffer, bytes_received);
    }

    if (IS_A_TTY(FILENO(stdout)) && response.starts_with("Error: ")) {
        std::cerr << ANSI_RED_TEXT << response << ANSI_RESET;
    } else {
        std::cout << response;
    }

    std::cout.flush();

    close(server_socket);
}

std::optional<std::string> Client::AwaitCommand() {
    std::string cmd;
    std::cout << "> ";

    std::getline(std::cin, cmd);

    if (!cmd.size()) {
        return std::nullopt;
    }

    return cmd;
}
