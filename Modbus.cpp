#include "Modbus.h"
#include "FieldMapParser.h"
#include <algorithm>
#include <cassert>
#include <cerrno>
#include <cstdlib>
#include <iostream>
#include <modbus/modbus-tcp.h>
#include <modbus/modbus.h>
#include <poll.h>
#include <stdexcept>
#include <sys/poll.h>
#include <thread>
#include <unistd.h>
#include <vector>

ModbusServer::ModbusServer(FieldMap &map, std::string port) : map_(map) {
    modbus_context_ = modbus_new_tcp_pi(NULL, port.c_str());
    if (modbus_context_ == NULL)
        throw std::runtime_error("Failed to allocatate modbus context");

    modbus_set_slave(modbus_context_,
                     map.modbus_device_id.value_or(MODBUS_TCP_SLAVE));
}

ModbusServer::~ModbusServer() {
    Stop();
    if (modbus_context_)
        modbus_free(modbus_context_);
    if (modbus_mapping_)
        modbus_mapping_free(modbus_mapping_);
}

void ModbusServer::Start(unsigned int max_connections) {
    if (t_.joinable()) {
        std::cout << "Attempted to start modbus, but already running\n";
        return;
    }

    stop_ = false;
    nb_connections_ = max_connections;

    socket_ = modbus_tcp_pi_listen(modbus_context_, nb_connections_);

    if (socket_ == -1)
        throw std::runtime_error("Failed to bind modbus socket");

    t_ = std::thread([this]() { this->ServeContext(); });
}

void ModbusServer::ServeContext() {
    // -1 means unused
    std::vector<pollfd> pollfds(nb_connections_ + 1, pollfd{-1, POLLIN, 0});
    uint8_t request[MODBUS_TCP_MAX_ADU_LENGTH];

    // The listnening socket from libmodbus
    pollfds[0].fd = socket_;

    while (!stop_) {
        int result = poll(pollfds.data(), pollfds.size(), 100);
        if (result == -1) {
            if (errno == EINTR)
                continue;
            std::cerr << "Error polling connections.  Stopping modbus.\n";
            break;
        }

        if (result == 0)
            continue;

        // Check if we have any new connections on the listnening socket
        if (pollfds[0].revents & POLLIN) {
            int client = modbus_tcp_pi_accept(modbus_context_, &socket_);
            if (client != -1) {
                // Find an usused slot
                auto slot =
                    std::find_if(pollfds.begin() + 1, pollfds.end(),
                                 [](const pollfd &p) { return p.fd == -1; });

                if (slot != pollfds.end()) {
                    slot->fd = client;
                } else {
                    close(client); // Already at max connections
                }
            }
        }

        try {
            BuildModbusMapping();
        } catch (const std::runtime_error e) {
            std::cerr << e.what() << "\n";
            break;
        }

        // Respond to requests
        for (int i = 1; i < pollfds.size(); i++) {
            pollfd &p = pollfds[i];
            if (p.fd == -1 || p.revents == 0)
                continue;

            if (p.revents & (POLLHUP | POLLERR | POLLNVAL)) {
                close(p.fd);
                p.fd = -1;
                continue;
            }

            if (!(p.revents & POLLIN))
                continue;

            if (modbus_set_socket(modbus_context_, p.fd)) {
                std::cerr << "Failed to assign socket\n";
                continue;
            }

            int request_len = modbus_receive(modbus_context_, request);

            // Check for bad request
            if (request_len == -1) {
                close(p.fd);
                p.fd = -1;
                continue;
            }

            // libmodbus ignores some requests
            if (request_len == 0)
                continue;

            modbus_reply(modbus_context_, request, request_len,
                         modbus_mapping_);
        }
    }

    for (int i = 0; i < pollfds.size(); i++) {
        int &sock = pollfds[i].fd;
        if (sock != -1) {
            close(sock);
            sock = -1;
        }
    }
    socket_ = -1;
}

void ModbusServer::BuildModbusMapping() {
    if (!modbus_mapping_) {
        int highest_reg = 0;
        for (Field &field : map_.fields) {
            if (field.modbus && field.modbus.value().reg_end > highest_reg) {
                highest_reg = field.modbus.value().reg_end;
            }
        }
        modbus_mapping_ = modbus_mapping_new(0, 0, highest_reg + 1, 0);
        if (!modbus_mapping_) {
            throw std::runtime_error("Failed to allocate modbus mapping");
        }
    }

    for (Field &field : map_.fields) {
        if (!field.modbus)
            continue;

        int start = field.modbus.value().reg_start;
        int end = field.modbus.value().reg_end;

        switch (field.type) {
        case FieldType::String: {
            std::string value = std::get<std::string>(field.value);
            for (int i = 0; i <= end - start + 1; i++) {
                std::size_t pos = static_cast<std::size_t>(i) * 2;
                uint16_t packed = 0;

                if (pos < value.size()) {
                    packed = static_cast<uint16_t>(
                                 static_cast<unsigned char>(value[pos]))
                             << 8;
                }

                if (pos + 1 < value.size()) {
                    packed |= static_cast<unsigned char>(value[pos + 1]);
                }

                modbus_mapping_->tab_registers[start + i] = packed;
            }
            break;
        }
        case FieldType::Int16:
            modbus_mapping_->tab_registers[start] =
                static_cast<std::uint16_t>(std::get<std::int16_t>(field.value));
            break;
        }
    }
}

void ModbusServer::Stop() {
    stop_ = true;
    if (t_.joinable()) {
        t_.join();
    }
}
