#ifndef H_SERVER
#define H_SERVER

#include "CommandParser.h"
#include "Modbus.h"
#include <atomic>
#include <string>

struct ServerConfig {
    std::string map_path;
    bool is_modbus_enabled = false;
    bool is_bacnet_enabled = false;
    int max_modbus_connections = MAX_MODBUS_CONNECTIONS;
    int modbus_port;
    int port;
};

class Server {
  public:
    const int max_command_len = 1024;
    const int max_connections = 10;

    Server(ServerConfig config, const std::atomic<bool> &stop);
    void Operate();

  private:
    ServerConfig config_;
    const std::atomic<bool> &stop_;

    void AcceptTCPRequests(CommandParser &cmd_parser);
};

#endif // H_SERVER
