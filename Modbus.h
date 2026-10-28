#ifndef H_MODBUS
#define H_MODBUS

#include "FieldMapParser.h"
#include <atomic>
#include <cstdint>
#include <modbus/modbus-tcp.h>
#include <modbus/modbus.h>
#include <string>
#include <thread>

#define MAX_MODBUS_CONNECTIONS 10

class ModbusServer {
  public:
    ModbusServer(FieldMap &map, std::string port = "502");

    ~ModbusServer();

    void Start();
    void Stop();
    bool IsActive();
    int GetMaxConnections();
    void SetMaxConnections(int conns);
    std::string Port();

  private:
    FieldMap &map_;
    modbus_t *modbus_context_ = nullptr;
    modbus_mapping_t *modbus_mapping_ = nullptr;
    std::string port_;
    int socket_ = -1;
    int nb_connections_ = MAX_MODBUS_CONNECTIONS;
    std::thread t_;
    std::atomic<bool> stop_{false};

    void ServeContext();
    void BuildModbusMapping();
    void PackUint32(std::uint32_t value, int start);
};

#endif // !H_MODBUS
