#ifndef H_MODBUS
#define H_MODBUS

#include "FieldMapParser.h"
#include <modbus/modbus-tcp.h>
#include <modbus/modbus.h>
#include <string>

class ModbusServer {
  public:
    ModbusServer(FieldMap &map, std::string port = "502");

    ~ModbusServer();

    void Start(int max_connections = 1);
    void Stop();

  private:
    FieldMap &map_;
    modbus_t *modbus_context_;
    int socket_;
    int nb_connections;
};

#endif // !H_MODBUS
