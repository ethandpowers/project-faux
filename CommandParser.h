#ifndef H_COMMAND_PARSER
#define H_COMMAND_PARSER

#include "Bacnet.h"
#include "FieldMapParser.h"
#include "Modbus.h"
#include <memory>
#include <optional>
#include <string>

class Command {
  public:
    virtual std::optional<std::string> Execute() = 0;
    virtual ~Command() = default;
};

class CommandParser {
  public:
    CommandParser(FieldMap &map, ModbusServer &modbus, BacnetServer &bacnet);
    std::unique_ptr<Command> ParseCommand(const std::string &cmd);

  private:
    FieldMap &map_;
    ModbusServer &modbus_;
    BacnetServer &bacnet_;
};

#endif // !H_COMMAND_PARSER
