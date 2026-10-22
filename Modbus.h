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
    modbus_t *modbus_context_ = nullptr;
    modbus_mapping_t *modbus_mapping_ = nullptr;
    int socket_ = -1;
    int nb_connections_ = 1;
    std::thread t_;
    std::atomic<bool> stop_{false};

    void ServeContext();
    void BuildModbusMapping();
};

#endif // !H_MODBUS
