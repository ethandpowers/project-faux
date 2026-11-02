#include "Server.h"
#include "Bacnet.h"
#include "FieldMapParser.h"
#include "Modbus.h"
#include <cerrno>
#include <cstring>
#include <exception>
#include <iostream>
#include <netinet/in.h>
#include <pthread.h>
#include <stdexcept>
#include <string>
#include <sys/poll.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <utility>
#include <vector>

Server::Server(ServerConfig config) : config_(std::move(config)) {}

struct Client {
    sockaddr_in client_address;
    socklen_t client_length;
    std::string input;
};

static void StartTCPServer() {
    int port = 1492;
    int server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket == -1)
        throw std::runtime_error("Failed to create socket.");

    int opt = 1;
    setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    sockaddr_in server_address{};
    server_address.sin_family = AF_INET;
    server_address.sin_addr.s_addr = INADDR_ANY;
    server_address.sin_port = htons(port);

    if (bind(server_socket, (sockaddr *)&server_address,
             sizeof(server_address))) {
        close(server_socket);
        throw std::runtime_error("Failed to bind socket.");
    }

    if (listen(server_socket, 5) == -1) {
        close(server_socket);
        throw std::runtime_error("Failed to listen on socket.");
    }

    std::cout << "Listening on " << port << "\n";

    const int max_connections = 10;
    std::vector<Client> clients(max_connections);
    std::vector<pollfd> pollfds(max_connections + 1, pollfd{-1, POLLIN, 0});
    pollfds[0].fd = server_socket;

    char buffer[1024];

    while (true) {
        int result = poll(pollfds.data(), pollfds.size(), 100);
        if (result == -1) {
            if (errno == EINTR)
                continue;
            std::cerr << "Error polling connections.\n";
            sleep(1);
            continue;
        }

        if (result == 0)
            continue;

        // Check if any new connections
        if (pollfds[0].revents & POLLIN) {
            std::cout << "Incoming connection\n";
            int fd_index = -1;
            for (int i = 1; i <= max_connections; i++) {
                if (pollfds[i].fd == -1) {
                    fd_index = i;
                    break;
                }
            }

            if (fd_index == -1) {
                std::cerr
                    << "Max connections reached.  Closing new connection.\n";
                int new_conn = accept(server_socket, nullptr, nullptr);
                if (new_conn != -1)
                    close(new_conn);
            } else {
                Client &client = clients[fd_index - 1];
                client.client_length = sizeof(sockaddr_in);
                int client_socket =
                    accept(server_socket, (sockaddr *)&(client.client_address),
                           &(client.client_length));

                if (client_socket == -1) {
                    std::cerr << "Accept failed\n";
                } else {
                    pollfds[fd_index].fd = client_socket;
                }
            }
        }

        // Respond to requests
        for (int i = 1; i < pollfds.size(); i++) {
            pollfd &p = pollfds[i];
            if (p.fd == -1 || p.revents == 0)
                continue;

            if (p.revents & (POLLHUP | POLLERR | POLLNVAL)) {
                std::cout << "Closing connection\n";
                close(p.fd);
                p.fd = -1;
                continue;
            }

            if (!(p.revents & POLLIN))
                continue;

            std::memset(buffer, 0, sizeof(buffer));

            ssize_t bytes_received = recv(p.fd, &buffer, sizeof(buffer) - 1, 0);

            if (bytes_received == -1) {
                std::cerr << "Receive error\n";
                continue;
            }

            if (bytes_received == 0) {
                close(p.fd);
                p.fd = -1;
                continue;
            }

            std::cout << buffer << "\n";
            send(p.fd, buffer, bytes_received, 0);
        }
    }

    for (int i = 0; i < pollfds.size(); i++) {
        int &sock = pollfds[i].fd;
        if (sock != -1) {
            close(sock);
            sock = -1;
        }
    }
}

void Server::Operate() {
    FieldMapParser parser;
    FieldMap map = parser.Parse(config_.map_path);

    ModbusServer modbus(map);
    modbus.SetMaxConnections(config_.max_modbus_connections);

    BacnetServer bacnet(map);

    if (config_.is_modbus_enabled) {
        std::cout << "Starting modbus server\n";
        try {
            modbus.Start();
        } catch (std::exception &e) {
            std::cerr << e.what() << "\n";
        }
    }

    if (config_.is_bacnet_enabled) {
        std::cout << "Starting BACnet server\n";
        try {
            bacnet.Start();
        } catch (std::exception &e) {
            std::cerr << e.what() << "\n";
        }
    }

    StartTCPServer();

    std::cout << "Shutting down\n";
    modbus.Stop();
    bacnet.Stop();
}
