#include "Bacnet.h"
#include "FieldMapParser.h"
#include "Modbus.h"

int main() {
    FieldMapParser parser;
    FieldMap map = parser.Parse("./maps/sample1.yaml");

    map.Print();

    ModbusServer modbus(map);
    BacnetServer bacnet(map);

    modbus.Start();
    bacnet.Start();

    while (true) {
    }
    return 0;
}
