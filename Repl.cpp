#include "Repl.h"
#include "Bacnet.h"
#include "Modbus.h"
#include <iostream>
#include <iterator>
#include <memory>
#include <optional>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

static std::vector<std::string> Tokenize(const std::string &text) {
    std::istringstream stream(text);
    return {std::istream_iterator<std::string>(stream),
            std::istream_iterator<std::string>()};
}

class HelpCommand : public Command {
  public:
    void Execute() override { std::cout << "No one can help you now\n"; }
};

class ModbusCommand : public Command {
  public:
    ModbusCommand(ModbusServer &server, const std::vector<std::string> &args)
        : server_(server) {
        if (args.size() != 2)
            throw std::runtime_error("Command \"modbus\" expects one of "
                                     "\"start\", \"stop\", \"port\"");

        if (args[1] == "start") {
            subcommand_ = Subcommand::Start;
        } else if (args[1] == "stop") {
            subcommand_ = Subcommand::Stop;
        } else if (args[1] == "port") {
            subcommand_ = Subcommand::Port;
        } else {
            throw std::runtime_error("Unknown subcommand: " + args[1]);
        }
    }

    void Execute() override {
        switch (subcommand_) {
        case Subcommand::Start:
            if (server_.IsActive())
                throw std::runtime_error("Modbus server already active on " +
                                         server_.Port());

            server_.Start(server_.MaxConnections());
            break;
        case Subcommand::Stop:
            if (!server_.IsActive())
                throw std::runtime_error("Modbus server not active");

            server_.Stop();
            break;
        case Subcommand::Port:
            std::cout << server_.Port() << "\n";
            break;
        }
    }

  private:
    enum class Subcommand { Start, Stop, Port };

    ModbusServer &server_;
    Subcommand subcommand_;
};

Repl::Repl(ModbusServer &modbus, BacnetServer &bacnet)
    : modbus_(modbus), bacnet_(bacnet) {}

void Repl::Operate() {
    std::cout << "Welcome to the repl!  Enter \"help\" for assistance.\n";

    while (auto cmd_str = AwaitCommand()) {
        try {
            if (auto cmd = ParseCommand(*cmd_str))
                cmd->Execute();
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

std::unique_ptr<Command> Repl::ParseCommand(const std::string &cmd) {
    auto args = Tokenize(cmd);

    if (args.empty())
        return nullptr;

    if (args[0] == "help") {
        return std::make_unique<HelpCommand>();
    } else if (args[0] == "modbus") {
        return std::make_unique<ModbusCommand>(modbus_, args);
    }

    throw std::runtime_error("Unknown command: " + args[0]);
}
