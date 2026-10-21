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
        } else if (type == "int") {
            field.type = FieldType::Int;
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

        if (fieldYaml["default"]) {
            if (!fieldYaml["default"].IsScalar()) {
                throw std::runtime_error("Invalid field map: Default for " +
                                         field.name + " is non-scalar");
            }

            YAML::Node defaultNode = fieldYaml["default"];

            switch (field.type) {
            case FieldType::String:
                field.value = defaultNode.as<std::string>();
                break;
            case FieldType::Int:
                field.value = defaultNode.as<std::int32_t>();
                break;
            case FieldType::Float:
                field.value = defaultNode.as<float>();
                break;
            case FieldType::Double:
                field.value = defaultNode.as<double>();
                break;
            case FieldType::Bool:
                field.value = defaultNode.as<bool>();
                break;
            default:
                throw std::runtime_error(
                    "Bug: Unhandled default for FieldType");
            }
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

                field.modbus = ModbusOpts{
                    .reg_start = fieldYaml["modbus"]["start"].as<int>(),
                    .reg_end = fieldYaml["modbus"]["end"].as<int>()};
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
    for (Field field : this->fields) {
        std::cout << "Name: " << field.name << "\n";
        std::cout << "Type: " << field_type_to_string(field.type) << "\n";
        std::cout << "Default: " << field_value_to_string(field.value) << "\n";
        if (field.modbus) {
            std::cout << "Modbus reg: " << field.modbus->reg_start << "-"
                      << field.modbus->reg_end << "\n";
        }

        if (field.bacnet) {
            std::cout << "BACnet instance: " << field.bacnet->instance << "\n";
        }

        std::cout << "\n";
    }
}

std::string field_type_to_string(FieldType type) {
    switch (type) {
    case FieldType::String:
        return "string";
    case FieldType::Int:
        return "int";
    case FieldType::Float:
        return "float";
    case FieldType::Double:
        return "double";
    case FieldType::Bool:
        return "bool";
    }

    return "unknown";
}

std::string field_value_to_string(FieldValue value) {
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
