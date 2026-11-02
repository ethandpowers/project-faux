#include "Repl.h"
#include "Bacnet.h"
#include "CommandParser.h"
#include "FieldMapParser.h"
#include "Modbus.h"
#include <iostream>
#include <optional>

Repl::Repl(FieldMap &map, ModbusServer &modbus, BacnetServer &bacnet)
    : map_(map), modbus_(modbus), bacnet_(bacnet) {}

void Repl::Operate() {
    std::cout << "Welcome to the repl!  Enter \"help\" for assistance.\n";

    CommandParser parser(map_, modbus_, bacnet_);

    while (auto cmd_str = AwaitCommand()) {
        try {
            if (auto cmd = parser.ParseCommand(*cmd_str)) {
                std::optional<std::string> out = cmd->Execute();

                if (out) {
                    std::cout << out.value() << "\n";
                }
            }
        } catch (const std::exception &e) {
            std::cerr << e.what() << "\n";
        }
    }
}

std::optional<std::string> Repl::AwaitCommand() {
    std::string cmd;
    std::cout << "> ";

    if (!std::getline(std::cin, cmd)) {
        std::cout << "\n";
        return std::nullopt;
    }

    return cmd;
}
