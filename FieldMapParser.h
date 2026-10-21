#ifndef H_FIELD_MAP_PARSER
#define H_FIELD_MAP_PARSER

#include <cstdint>
#include <optional>
#include <string>
#include <variant>
#include <vector>

enum class FieldType {
    String,
    Int,
    Float,
    Double,
    Bool,
};

using FieldValue = std::variant<std::string, std::int32_t, float, double, bool>;

struct ModbusOpts {
    int modbus_start;
    int modbus_end; // Even though this field is optional from the user's
                    // perspective, it will be inferred here
};

struct BacnetOpts {
    int bacnet_instance;
};

struct Field {
    std::string name;
    FieldType type;
    FieldValue value;
    std::optional<ModbusOpts> modbus;
    std::optional<BacnetOpts> bacnet;
};

struct FieldMap {
    std::string name;
    std::optional<uint8_t> modbus_device_id;
    std::optional<int32_t> bacnet_device_id;
    std::vector<Field> fields;

    void Print();
};

class FieldMapParser {
  public:
    FieldMap Parse(std::string path);
};

std::string field_type_to_string(FieldType type);

#endif // !H_FIELD_MAP_PARSER
