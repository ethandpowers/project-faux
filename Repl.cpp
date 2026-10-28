#include "Repl.h"
#include "Bacnet.h"
#include "Modbus.h"
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>

class HelpCommand : public Command {
  public:
    void Execute() override { std::cout << "No one can help you now\n"; }
};

Repl::Repl(ModbusServer &modbus, BacnetServer &bacnet)
    : modbus_(modbus), bacnet_(bacnet) {}

void Repl::Operate() {
    std::cout << "Welcome to the repl!  Enter \"help\" for assistance.\n";

    while (true) {
        std::string cmd_str = AwaitCommand();
        try {
            auto cmd = ParseCommand(cmd_str);
            cmd->Execute();
        } catch (const std::exception &e) {
            std::cerr << e.what() << "\n";
        }
    }
}

std::string Repl::AwaitCommand() {
    std::string cmd;
    std::cout << "> ";
    std::getline(std::cin, cmd);

    return cmd;
}

std::unique_ptr<Command> Repl::ParseCommand(std::string cmd) {
    if (cmd == "help") {
        return std::make_unique<HelpCommand>();
    }

    throw std::runtime_error("Unknown command: " + cmd);
}
