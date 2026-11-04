#include "Client.h"
#include <arpa/inet.h>
#include <iostream>
#include <netdb.h>
#include <netinet/in.h>
#include <stdexcept>
#include <string>
#include <sys/socket.h>
#include <unistd.h>

std::string ClientConfig::ServerStr() {
    return this->server + ":" + std::to_string(this->port);
}

Client::Client(ClientConfig config) : config_(config) {};

void Client::SendCommand() {
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

    std::string message = config_.command + "\n";

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

    std::cout << response;

    close(server_socket);
}
