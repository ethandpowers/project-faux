#include "FieldMapParser.h"
#include <iostream>
#include <stdexcept>
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
