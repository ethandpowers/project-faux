#include "Bacnet.h"
#include "FieldMapParser.h"
#include "Modbus.h"
#include "Repl.h"
#include <argparse/argparse.hpp>

int main(int argc, char *argv[]) {
    argparse::ArgumentParser program("faux");

    program.add_argument("field_map")
        .help("Supply the location of the field map");

    program.add_argument("-m", "--modbus")
        .help("Turn on the modbus server")
        .default_value(false)
        .implicit_value(true);

    program.add_argument("-b", "--bacnet")
        .help("Turn on the BACnet server")
        .default_value(false)
        .implicit_value(true);

    try {
        program.parse_args(argc, argv);
    } catch (const std::exception &err) {
        std::cerr << err.what() << std::endl;
        std::cerr << program;
        return 1;
    }

    std::string map_path = program.get("field_map");
    bool is_modbus_enabled = program.get<bool>("-m");
    bool is_bacnet_enabled = program.get<bool>("-b");

    FieldMapParser parser;
    FieldMap map = parser.Parse(map_path);

    ModbusServer modbus(map);
    BacnetServer bacnet(map);

    if (is_modbus_enabled) {
        std::cout << "Starting modbus server\n";
        modbus.Start();
    }

    if (is_bacnet_enabled) {
        std::cout << "Starting BACnet server\n";
        bacnet.Start();
    }

    std::cout << "Service(s) online\n";
    Repl repl(modbus, bacnet);

    repl.Operate();

    return 0;
}
