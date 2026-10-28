#ifndef H_REPL
#define H_REPL

#include "Bacnet.h"
#include "Modbus.h"

class Repl {
  public:
    Repl(ModbusServer &modbus, BacnetServer &bacnet);
    void Operate();

  private:
    ModbusServer &modbus_;
    BacnetServer &bacnet_;

    std::string AwaitCommand();
};

#endif // !H_REPL
