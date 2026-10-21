#include "FieldMapParser.h"

int main() {
    FieldMapParser parser;
    FieldMap map = parser.Parse("./maps/sample1.yaml");

    map.Print();
    return 0;
}
