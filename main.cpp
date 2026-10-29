#include "Modbus.h"
#include "Server.h"
#include <argparse/argparse.hpp>
#include <exception>
#include <iostream>

int main(int argc, char *argv[]) {
    argparse::ArgumentParser program("faux");

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

    program.add_subparser(serve_command);

    try {
        program.parse_args(argc, argv);
    } catch (const std::exception &err) {
        std::cerr << err.what() << std::endl;
        std::cerr << program;
        return 1;
    }

    if (program.is_subcommand_used(serve_command)) {
        ServerConfig config;
        config.map_path = serve_command.get("field_map");
        config.is_modbus_enabled = serve_command.get<bool>("-m");
        config.is_bacnet_enabled = serve_command.get<bool>("-b");
        config.max_modbus_connections = serve_command.get<int>("-mc");

        try {
            Server server(config);
            server.Operate();
            // If server returns, then continue as a client
        } catch (const std::exception &err) {
            std::cerr << err.what() << std::endl;
            return 1;
        }
    }
    // Repl repl(map, modbus, bacnet);
    // repl.Operate();

    return 0;
}
