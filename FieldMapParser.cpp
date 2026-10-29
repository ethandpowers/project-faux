#include "FieldMapParser.h"
#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <variant>
#include <yaml-cpp/node/node.h>
#include <yaml-cpp/node/parse.h>
#include <yaml-cpp/node/type.h>
#include <yaml-cpp/yaml.h>

FieldMap FieldMapParser::Parse(std::string path) {
    FieldMap map;

    YAML::Node input = YAML::LoadFile(path);

    if (!input["name"]) {
        throw std::runtime_error("Invalid field map: Missing device name");
    }

    map.name = input["name"].as<std::string>();

    if (input["modbus_device_id"]) {
        map.modbus_device_id =
            input["modbus_device_id"]
                .as<decltype(map.modbus_device_id)::value_type>();
    }

    if (input["bacnet_device_id"]) {
        map.bacnet_device_id =
            input["bacnet_device_id"]
                .as<decltype(map.bacnet_device_id)::value_type>();
    }

    if (!input["fields"]) {
        throw std::runtime_error("Invalid field map: Missing fields");
    }

    if (!input["fields"].IsSequence()) {
        throw std::runtime_error(
            "Invalid field map: Expected fields to be a sequence");
    }

    for (int i = 0; i < input["fields"].size(); i++) {
        Field field;
        YAML::Node fieldYaml = input["fields"][i];

        if (!fieldYaml["name"]) {
            throw std::runtime_error(
                "Invalid field map: Field is missing name");
        }

        field.name = fieldYaml["name"].as<std::string>();

        if (!fieldYaml["type"]) {
            throw std::runtime_error(
                "Invalid field map: Missing field type for " + field.name);
        }
        if (!fieldYaml["type"].IsScalar()) {
            throw std::runtime_error("Invalid field map: Field type for " +
                                     field.name + " is non-scalar");
        }

        std::string type = fieldYaml["type"].as<std::string>();

        if (type == "string") {
            field.type = FieldType::String;
        } else if (type == "int16") {
            field.type = FieldType::Int16;
        } else if (type == "int32") {
            field.type = FieldType::Int32;
        } else if (type == "uint16") {
            field.type = FieldType::Uint16;
        } else if (type == "uint32") {
            field.type = FieldType::Uint32;
        } else if (type == "float") {
            field.type = FieldType::Float;
        } else if (type == "double") {
            field.type = FieldType::Double;
        } else if (type == "bool") {
            field.type = FieldType::Bool;
        } else {
            throw std::runtime_error(
                "Invalid field map: Invalid field type for " + field.name);
        }

        YAML::Node defaultNode = fieldYaml["default"];

        switch (field.type) {
        case FieldType::String:
            field.value = defaultNode.as<std::string>("");
            break;
        case FieldType::Int16:
            field.value = defaultNode.as<std::int16_t>(0);
            break;
        case FieldType::Int32:
            field.value = defaultNode.as<std::int32_t>(0);
            break;
        case FieldType::Uint16:
            field.value = defaultNode.as<std::uint16_t>(0);
            break;
        case FieldType::Uint32:
            field.value = defaultNode.as<std::uint32_t>(0);
            break;
        case FieldType::Float:
            field.value = defaultNode.as<float>(0.0);
            break;
        case FieldType::Double:
            field.value = defaultNode.as<double>(0.0);
            break;
        case FieldType::Bool:
            field.value = defaultNode.as<bool>(false);
            break;
        default:
            throw std::runtime_error("Bug: Unhandled default for FieldType");
        }

        if (fieldYaml["modbus"]) {
            if (fieldYaml["modbus"].IsScalar()) {
                int reg = fieldYaml["modbus"].as<int>();
                field.modbus = ModbusOpts{.reg_start = reg, .reg_end = reg};
            } else {
                if (!fieldYaml["modbus"]["start"] ||
                    !fieldYaml["modbus"]["end"]) {
                    throw std::runtime_error(
                        "Invalid field map: Malformed modbus options");
                }

                int start = fieldYaml["modbus"]["start"].as<int>();
                int end = fieldYaml["modbus"]["end"].as<int>();

                if (start > end)
                    throw std::runtime_error("Invalid field map: Modbus start "
                                             "greater than end for " +
                                             field.name);

                field.modbus = ModbusOpts{.reg_start = start, .reg_end = end};
            }

            int start = field.modbus.value().reg_start;
            int end = field.modbus.value().reg_end;

            if (!IsValidReg(start) || !IsValidReg(end))
                throw std::runtime_error(
                    "Invalid field map: Invalid modbus register for field " +
                    field.name);

            // Validate that fields fit in said registers
            int num_registers = 1 + field.modbus.value().reg_end -
                                field.modbus.value().reg_start;
            int min_registers = (FieldTypeBytes(field.type) + 1) / 2;

            if (num_registers < min_registers) {
                throw std::runtime_error(
                    "Invalid field map: Need at least " +
                    std::to_string(min_registers) + " registers for type " +
                    FieldTypeToString(field.type) + " in field " + field.name);
            }
        }

        if (fieldYaml["bacnet"]) {
            field.bacnet =
                BacnetOpts{.instance = fieldYaml["bacnet"].as<int>()};
        }

        map.fields.push_back(field);
    }

    return map;
}

bool FieldMapParser::IsValidReg(int reg) { return reg >= 0 && reg <= 65535; }

void FieldMap::Print() {
    std::cout << "Name: " << this->name << "\n";
    if (this->modbus_device_id) {
        std::cout << "Modbus Device Id: " << +this->modbus_device_id.value()
                  << "\n";
    }
    if (this->bacnet_device_id) {
        std::cout << "BACnet Device Id: " << +this->bacnet_device_id.value()
                  << "\n";
    }

    std::cout << "Fields:\n";
    for (Field &field : this->fields) {
        field.Print();
    }
}

void Field::Print() {
    std::cout << "Name: " << this->name << "\n";
    std::cout << "Type: " << FieldTypeToString(this->type) << "\n";
    std::cout << "Default: " << FieldValueToString(this->value) << "\n";
    if (this->modbus) {
        std::cout << "Modbus reg: " << this->modbus->reg_start << "-"
                  << this->modbus->reg_end << "\n";
    }

    if (this->bacnet) {
        std::cout << "BACnet instance: " << this->bacnet->instance << "\n";
    }

    std::cout << "\n";
}

int FieldTypeBytes(FieldType type) {
    switch (type) {
    case FieldType::String:
        return 1;
    case FieldType::Int16:
        return sizeof(std::int16_t);
    case FieldType::Int32:
        return sizeof(std::int32_t);
    case FieldType::Uint16:
        return sizeof(std::uint16_t);
    case FieldType::Uint32:
        return sizeof(std::uint32_t);
    case FieldType::Float:
        return sizeof(float);
    case FieldType::Double:
        return sizeof(double);
    case FieldType::Bool:
        return sizeof(bool);
    }

    return 1;
}

std::string FieldTypeToString(FieldType type) {
    switch (type) {
    case FieldType::String:
        return "string";
    case FieldType::Int16:
        return "int16";
    case FieldType::Int32:
        return "int32";
    case FieldType::Uint16:
        return "uint16";
    case FieldType::Uint32:
        return "uint32";
    case FieldType::Float:
        return "float";
    case FieldType::Double:
        return "double";
    case FieldType::Bool:
        return "bool";
    }

    return "unknown";
}

std::string FieldValueToString(FieldValue value) {
    return std::visit(
        [](const auto &arg) -> std::string {
            using T = std::decay_t<decltype(arg)>;
            if constexpr (std::is_same_v<T, std::string>) {
                return arg;
            } else if constexpr (std::is_same_v<T, bool>) {
                return arg ? "true" : "false";
            } else {
                return std::to_string(arg);
            }
        },
        value);
}
