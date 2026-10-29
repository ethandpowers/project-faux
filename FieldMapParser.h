#ifndef H_FIELD_MAP_PARSER
#define H_FIELD_MAP_PARSER

#include <cstdint>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <variant>
#include <vector>

enum class FieldType {
    String,
    Int16,
    Int32,
    Uint16,
    Uint32,
    Float,
    Double,
    Bool,
};

using FieldValue =
    std::variant<std::string, std::int16_t, std::int32_t, std::uint16_t,
                 std::uint32_t, float, double, bool>;

struct ModbusOpts {
    int reg_start;
    int reg_end; // Even though this field is optional from the user's
                 // perspective, it will be inferred here
};

struct BacnetOpts {
    int instance;
    bool is_dirty = true;
};

struct Field {
    std::string name;
    FieldType type;
    FieldValue value;
    std::optional<ModbusOpts> modbus;
    std::optional<BacnetOpts> bacnet;

    void Print();
};

struct FieldMap {
    std::string name;
    std::optional<uint8_t> modbus_device_id;
    std::optional<int32_t> bacnet_device_id;
    std::vector<Field> fields;
    std::unique_ptr<std::mutex> mutex = std::make_unique<std::mutex>();

    void Print();
};

class FieldMapParser {
  public:
    FieldMap Parse(std::string path);
    bool IsValidReg(int reg);
};

int FieldTypeBytes(FieldType type);
std::string FieldTypeToString(FieldType type);
std::string FieldValueToString(FieldValue value);

#endif // !H_FIELD_MAP_PARSER
