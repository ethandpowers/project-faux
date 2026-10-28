#ifndef H_REPL
#define H_REPL

#include "Bacnet.h"
#include "Modbus.h"
#include <memory>
#include <optional>
#include <string>

class Command {
  public:
    virtual void Execute() = 0;
    virtual ~Command() = default;
};

class Repl {
  public:
    Repl(ModbusServer &modbus, BacnetServer &bacnet);
    void Operate();

  private:
    ModbusServer &modbus_;
    BacnetServer &bacnet_;

    std::optional<std::string> AwaitCommand();
    std::unique_ptr<Command> ParseCommand(const std::string &cmd);
};

#endif // !H_REPL
