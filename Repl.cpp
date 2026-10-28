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
    void Execute() override {
        std::cout << "Help me, Obi-Wan Kenobi.  You're my only hope.\n";
    }
};

class ModbusCommand : public Command {
  public:
    ModbusCommand(ModbusServer &server, const std::vector<std::string> &args)
        : server_(server), args_(args) {
        if (args.size() < 2)
            throw std::runtime_error("Command \"modbus\" expects a subcommand");
    }

    void Execute() override {
        if (args_[1] == "start") {
            if (server_.IsActive())
                throw std::runtime_error("Modbus server already active on " +
                                         server_.Port());

            server_.Start();
        } else if (args_[1] == "stop") {
            if (!server_.IsActive())
                throw std::runtime_error("Modbus server not active");

            server_.Stop();
        } else if (args_[1] == "get") {
            if (args_.size() < 3) {
                throw std::runtime_error("Command \"get\" requires argument");
            }
            if (args_[2] == "port") {
                std::cout << server_.Port() << "\n";
            } else if (args_[2] == "max-connections") {
                std::cout << server_.GetMaxConnections() << "\n";
            } else {
                throw std::runtime_error("Unknown property: " + args_[2]);
            }
        } else if (args_[1] == "set") {
            if (args_.size() < 4) {
                throw std::runtime_error(
                    "Command \"set\" requires 2 arguments");
            }

            if (args_[2] == "max-connections") {
                server_.SetMaxConnections(std::stoi(args_[3]));
            } else {
                throw std::runtime_error("Unknown property: " + args_[2]);
            }
        } else {
            throw std::runtime_error("Unknown subcommand: " + args_[1]);
        }
    }

  private:
    ModbusServer &server_;
    std::vector<std::string> args_;
};

class BacnetCommand : public Command {
  public:
    BacnetCommand(BacnetServer &server, const std::vector<std::string> &args)
        : server_(server), args_(args) {
        if (args.size() < 2)
            throw std::runtime_error("Command \"bacnet\" expects a subcommand");
    }

    void Execute() override {
        if (args_[1] == "start") {
            if (server_.IsActive())
                throw std::runtime_error("BACnet server already active");

            server_.Start();
        } else if (args_[1] == "stop") {
            if (!server_.IsActive())
                throw std::runtime_error("BACnet server not active");

            server_.Stop();
        } else {
            throw std::runtime_error("Unknown subcommand: " + args_[1]);
        }
    }

  private:
    BacnetServer &server_;
    std::vector<std::string> args_;
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
    } else if (args[0] == "bacnet") {
        return std::make_unique<BacnetCommand>(bacnet_, args);
    }

    throw std::runtime_error("Unknown command: " + args[0]);
}
