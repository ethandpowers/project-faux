#ifndef H_SERVER
#define H_SERVER

#include "Modbus.h"
#include <string>

struct ServerConfig {
    std::string map_path;
    bool is_modbus_enabled = false;
    bool is_bacnet_enabled = false;
    int max_modbus_connections = MAX_MODBUS_CONNECTIONS;
};

class Server {
  public:
    explicit Server(ServerConfig config);
    void Operate();

  private:
    ServerConfig config_;
};

#endif // H_SERVER
