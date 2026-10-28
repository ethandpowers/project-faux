#include "Repl.h"
#include "Bacnet.h"
#include "Modbus.h"
#include <iostream>
#include <string>

Repl::Repl(ModbusServer &modbus, BacnetServer &bacnet)
    : modbus_(modbus), bacnet_(bacnet) {}

void Repl::Operate() {
    std::cout << "Welcome to the repl!  Enter \"help\" for assistance.\n\n";

    while (true) {
        std::string cmd = AwaitCommand();

        if (cmd == "help") {
            std::cout << "No one can help you now\n";
        } else {
            std::cout << cmd << "\n";
        }
    }
}

std::string Repl::AwaitCommand() {
    std::string cmd;
    std::cout << "> ";
    std::getline(std::cin, cmd);

    return cmd;
}
