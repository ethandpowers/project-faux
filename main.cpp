#include "FieldMapParser.h"
#include "Modbus.h"

int main() {
    FieldMapParser parser;
    FieldMap map = parser.Parse("./maps/sample1.yaml");

    map.Print();

    ModbusServer modbus(map);

    modbus.Start();

    while (true) {
    }
    return 0;
}
