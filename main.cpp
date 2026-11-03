#include "Modbus.h"
#include "Server.h"
#include <argparse/argparse.hpp>
#include <atomic>
#include <csignal>
#include <exception>
#include <iostream>

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
    argparse::ArgumentParser program("faux");

    // Shared argument
    program.add_argument("-p", "--port")
        .help("Specify server port")
        .default_value(1492)
        .scan<'i', int>();

    // Add server arguments
    argparse::ArgumentParser serve_command("serve");

    serve_command.add_argument("field_map")
        .help("Supply the location of the field map");

    serve_command.add_argument("-m", "--modbus")
        .help("Turn on the modbus server")
        .default_value(false)
        .implicit_value(true);

    serve_command.add_argument("-b", "--bacnet")
        .help("Turn on the BACnet server")
        .default_value(false)
        .implicit_value(true);

    serve_command.add_argument("-mc", "--modbus-connections")
        .help("Maximum allowed modbus connections")
        .default_value(MAX_MODBUS_CONNECTIONS)
        .scan<'i', int>();

    serve_command.add_argument("-mp", "--modbus-port")
        .help("Specify the port for modbus")
        .default_value(502)
        .scan<'i', int>();

    program.add_subparser(serve_command);

    // Add client arguments
    program.add_argument("-s", "--server")
        .help("Target server for client")
        .default_value("localhost");

    program.add_argument("command")
        .help("Command to pass to the server")
        .remaining();

    try {
        program.parse_args(argc, argv);
    } catch (const std::exception &err) {
        std::cerr << err.what() << std::endl;
        std::cerr << program;
        return 1;
    }

    InstallSignalHandlers();

    int port = program.get<int>("-p");

    if (program.is_subcommand_used(serve_command)) {
        ServerConfig config;
        config.map_path = serve_command.get("field_map");
        config.is_modbus_enabled = serve_command.get<bool>("-m");
        config.is_bacnet_enabled = serve_command.get<bool>("-b");
        config.max_modbus_connections = serve_command.get<int>("-mc");
        config.modbus_port = serve_command.get<int>("-mp");
        config.port = port;

        try {
            Server server(config, g_stop);
            server.Operate();
        } catch (const std::exception &err) {
            std::cerr << err.what() << std::endl;
            return 1;
        }
    } else {
        // TODO: Implement client
        std::string server = program.get("-s");

        if (!program.present("command")) {
            std::cerr << "No command provided\n";
            return 1;
        }

        std::vector<std::string> rest =
            program.get<std::vector<std::string>>("command");

        std::string command;

        for (int i = 0; i < rest.size(); i++) {
            command += rest[i];
            if (i < rest.size() - 1) {
                command += " ";
            }
        }

        std::cout << server << ":" << port << " -> " << command << "\n";
    }

    return 0;
}
