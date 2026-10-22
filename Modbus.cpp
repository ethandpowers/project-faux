#include "Modbus.h"
#include "FieldMapParser.h"
#include <cassert>
#include <iostream>
#include <modbus/modbus-tcp.h>
#include <modbus/modbus.h>
#include <poll.h>
#include <stdexcept>
#include <sys/poll.h>
#include <thread>

ModbusServer::ModbusServer(FieldMap &map, std::string port) : map_(map) {
    modbus_context_ = modbus_new_tcp_pi(NULL, port.c_str());
}

ModbusServer::~ModbusServer() {
    Stop();
    if (modbus_context_) {
        modbus_free(modbus_context_);
    }
}

void ModbusServer::Start(unsigned int max_connections) {
    if (max_connections > 1) {
        throw std::runtime_error("Multiple connections not yet supported");
    }

    stop_ = false;
    nb_connections_ = max_connections;

    socket_ = modbus_tcp_pi_listen(modbus_context_, nb_connections_);

    if (socket_ == -1) {
        throw std::runtime_error("Failed to bind modbus socket");
    }

    t_ = std::thread([this]() { this->ServeContext(); });
}

void ModbusServer::ServeContext() {

    nfds_t num_connections;
    pollfd *pollfds =
        static_cast<pollfd *>(calloc(nb_connections_, sizeof(nfds_t)));
    if (!pollfds) {
        std::cout << "Failed to allocate modbus connections.  Stopping.\n";
        return;
    }

    while (!stop_) {
        // TODO: Respond to connections
    }
}

void ModbusServer::Stop() {
    stop_ = true;
    if (t_.joinable()) {
        t_.join();
    }

    if (socket_ != -1) {
        close(socket_);
        socket_ = -1;
    }
}
