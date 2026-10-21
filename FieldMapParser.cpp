#include "FieldMapParser.h"

#include <iostream>
#include <stdexcept>
#include <yaml-cpp/node/node.h>
#include <yaml-cpp/node/parse.h>
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
}
