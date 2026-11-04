#include "CLI/CLI.hpp"
#include "Client.h"
#include "Modbus.h"
#include "Server.h"
#include <atomic>
#include <csignal>
#include <exception>
#include <iostream>
#include <vector>

namespace {
std::atomic<bool> g_stop{false};
static_assert(std::atomic<bool>::is_always_lock_free);

void HandleSignal(int) { g_stop.store(true); }

void InstallSignalHandlers() {
    struct sigaction sa{};
    sa.sa_handler = HandleSignal;
    sigemptyset(&sa.sa_mask);
    sigaction(SIGINT, &sa, nullptr);
    sigaction(SIGTERM, &sa, nullptr);
}
} // namespace

int main(int argc, char *argv[]) {
    int port = 1492;
    std::string server = "localhost";
    std::vector<std::string> command_parts;
    bool is_repl = false;
    std::string field_map;
    bool is_modbus_enabled = false;
    bool is_bacnet_enabled = false;
    int modbus_port = 502;
    int max_modbus_connections = MAX_MODBUS_CONNECTIONS;

    CLI::App app("faux");
    app.fallthrough();

    app.add_option("-p,--port", port, "Specify server port")
        ->capture_default_str();

    // Client args
    auto server_opt =
        app.add_option("-s,--server", server, "Target server for client")
            ->capture_default_str();
    auto command_opt = app.add_option("command", command_parts);
    app.add_flag("-r,--repl", is_repl,
                 "Open a REPL isntead of sending a command")
        ->capture_default_str();

    // Server args
    CLI::App *server_cmd = app.add_subcommand("serve", "Create a new server");
    server_cmd->excludes(server_opt);
    server_cmd->excludes(command_opt);
    server_cmd->add_option("field_map", field_map, "Specify the field map")
        ->required();
    server_cmd->add_flag("-m,--modbus", is_modbus_enabled,
                         "Turn on the modbus server");
    server_cmd->add_flag("-b,--bacnet", is_bacnet_enabled,
                         "Turn on the BACnet server");
    server_cmd
        ->add_option("--modbus-port", modbus_port,
                     "Set the port for the modbus server")
        ->capture_default_str();
    server_cmd
        ->add_option("--modbus-connections", max_modbus_connections,
                     "Set the maximum number of modbus connections")
        ->capture_default_str();

    CLI11_PARSE(app, argc, argv);

    InstallSignalHandlers();

    if (*server_cmd) {
        ServerConfig config;
        config.map_path = field_map;
        config.is_modbus_enabled = is_modbus_enabled;
        config.is_bacnet_enabled = is_bacnet_enabled;
        config.max_modbus_connections = max_modbus_connections;
        config.modbus_port = modbus_port;
        config.port = port;

        try {
            Server server(config, g_stop);
            server.Operate();
        } catch (const std::exception &err) {
            std::cerr << err.what() << std::endl;
            return 1;
        }
    } else {
        if (!is_repl && command_parts.empty()) {
            std::cerr << "No command provided\n";
            return 1;
        }

        ClientConfig config;

        config.port = port;
        config.server = server;
        config.command = CLI::detail::join(command_parts, " ");
        config.is_repl = is_repl;

        Client client(config, g_stop);

        try {
            client.Operate();
        } catch (const std::exception &err) {
            std::cerr << err.what() << std::endl;
            return 1;
        }
    }

    return 0;
}
