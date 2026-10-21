#include "Modbus.h"
#include "FieldMapParser.h"
#include <modbus/modbus-tcp.h>
#include <modbus/modbus.h>

ModbusServer::ModbusServer(FieldMap &map, std::string port) : map_(map) {
    this->modbus_context_ = modbus_new_tcp_pi(NULL, port.c_str());
}

ModbusServer::~ModbusServer() {
    if (this->modbus_context_) {
        modbus_free(this->modbus_context_);
    }
}

void ModbusServer::Start(int max_connections) {
    this->nb_connections = max_connections;
    this->socket_ =
        modbus_tcp_pi_listen(this->modbus_context_, this->nb_connections);

    // TODO: use poll to multiplex TCP connections
}
