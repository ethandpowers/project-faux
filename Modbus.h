#ifndef H_MODBUS
#define H_MODBUS

#include "FieldMapParser.h"
#include <atomic>
#include <modbus/modbus-tcp.h>
#include <modbus/modbus.h>
#include <string>
#include <thread>

class ModbusServer {
  public:
    ModbusServer(FieldMap &map, std::string port = "502");

    ~ModbusServer();

    void Start(unsigned int max_connections = 1);
    void Stop();

  private:
    FieldMap &map_;
    modbus_t *modbus_context_;
    int socket_;
    int nb_connections_;
    std::thread t_;
    std::atomic<bool> stop_;

    void ServeContext();
};

#endif // !H_MODBUS
