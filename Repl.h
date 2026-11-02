#ifndef H_REPL
#define H_REPL

#include "Bacnet.h"
#include "FieldMapParser.h"
#include "Modbus.h"
#include <optional>
#include <string>

class Repl {
  public:
    Repl(FieldMap &map, ModbusServer &modbus, BacnetServer &bacnet);
    void Operate();

  private:
    FieldMap &map_;
    ModbusServer &modbus_;
    BacnetServer &bacnet_;

    std::optional<std::string> AwaitCommand();
};

#endif // !H_REPL
