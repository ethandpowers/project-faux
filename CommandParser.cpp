#include "CommandParser.h"
#include "Bacnet.h"
#include "FieldMapParser.h"
#include "Modbus.h"
#include <iterator>
#include <memory>
#include <mutex>
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
    std::optional<std::string> Execute() override {
        return "Help me, Obi-Wan Kenobi.  You're my only hope.";
    }
};

class ModbusCommand : public Command {
  public:
    ModbusCommand(ModbusServer &server, const std::vector<std::string> &args)
        : server_(server), args_(args) {
        if (args.size() < 2)
            throw std::runtime_error("Command \"modbus\" expects a subcommand");
    }

    std::optional<std::string> Execute() override {
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
                return server_.Port();
            } else if (args_[2] == "max-connections") {
                return std::to_string(server_.GetMaxConnections());
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
        } else if (args_[1] == "status") {
            return server_.IsActive() ? "online" : "offline";
        } else {
            throw std::runtime_error("Unknown subcommand: " + args_[1]);
        }

        return std::nullopt;
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

    std::optional<std::string> Execute() override {
        if (args_[1] == "start") {
            if (server_.IsActive())
                throw std::runtime_error("BACnet server already active");

            server_.Start();
        } else if (args_[1] == "stop") {
            if (!server_.IsActive())
                throw std::runtime_error("BACnet server not active");

            server_.Stop();
        } else if (args_[1] == "status") {
            return server_.IsActive() ? "online" : "offline";
        } else {
            throw std::runtime_error("Unknown subcommand: " + args_[1]);
        }

        return std::nullopt;
    }

  private:
    BacnetServer &server_;
    std::vector<std::string> args_;
};

class FieldCommand : public Command {
  public:
    FieldCommand(FieldMap &map, const std::vector<std::string> &args)
        : map_(map), args_(args) {
        if (args.size() < 2)
            throw std::runtime_error("Command \"field\" expects a subcommand");
    }

    std::optional<std::string> Execute() override {
        std::lock_guard<std::mutex> lock(*map_.mutex);

        if (args_[1] == "list") {
            std::string field_str;
            for (Field &field : map_.fields) {
                field_str.append(field.ToString());
            }
            return field_str;
        } else if (args_[1] == "get") {
            if (args_.size() < 3)
                throw std::runtime_error(
                    "Subcommand \"get\" expects a field name");

            std::string name = args_[2];
            for (Field &field : map_.fields) {
                if (field.name == name) {
                    return FieldValueToString(field.value);
                }
            }
            throw std::runtime_error("No such field: " + name);
        } else if (args_[1] == "set") {
            if (args_.size() < 4)
                throw std::runtime_error(
                    "Subcommand \"set\" expects a field name and value");

            std::string name = args_[2];
            std::string value = args_[3];
            for (Field &field : map_.fields) {
                if (field.name == name) {
                    switch (field.type) {
                    case FieldType::String:
                        field.value = value;
                        break;
                    case FieldType::Int16:
                        field.value =
                            static_cast<std::int16_t>(std::stoi(value));
                        break;
                    case FieldType::Int32:
                        field.value = std::stoi(value);
                        break;
                    case FieldType::Uint16:
                        field.value =
                            static_cast<std::uint16_t>(std::stoi(value));
                        break;
                    case FieldType::Uint32:
                        field.value =
                            static_cast<std::uint32_t>(std::stoi(value));
                        break;
                    case FieldType::Float:
                        field.value = std::stof(value);
                        break;
                    case FieldType::Double:
                        field.value = std::stod(value);
                        break;
                    case FieldType::Bool:
                        if (value != "false" && value != "true")
                            throw std::runtime_error("Invalid boolean value");
                        field.value = value == "true";
                    }
                    return std::nullopt;
                }
            }
            throw std::runtime_error("No such field: " + name);
        } else {
            throw std::runtime_error("Unknown subcommand: " + args_[1]);
        }

        return std::nullopt;
    }

  private:
    FieldMap &map_;
    std::vector<std::string> args_;
};

CommandParser::CommandParser(FieldMap &map, ModbusServer &modbus,
                             BacnetServer &bacnet)
    : map_(map), modbus_(modbus), bacnet_(bacnet) {}

std::unique_ptr<Command> CommandParser::ParseCommand(const std::string &cmd) {
    auto args = Tokenize(cmd);

    if (args.empty())
        return nullptr;

    if (args[0] == "help") {
        return std::make_unique<HelpCommand>();
    } else if (args[0] == "modbus") {
        return std::make_unique<ModbusCommand>(modbus_, args);
    } else if (args[0] == "bacnet") {
        return std::make_unique<BacnetCommand>(bacnet_, args);
    } else if (args[0] == "field") {
        return std::make_unique<FieldCommand>(map_, args);
    }

    throw std::runtime_error("Unknown command: " + args[0]);
}
