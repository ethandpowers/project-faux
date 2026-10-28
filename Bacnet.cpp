#include "Bacnet.h"
#include "FieldMapParser.h"
#include "bacnet/bacdef.h"
#include "bacnet/bacstr.h"
#include "bacnet/basic/object/iv.h"
#include "bacnet/basic/server/bacnet_basic.h"
#include "bacnet/config.h"
#include <bacnet/basic/object/av.h>
#include <bacnet/basic/object/bv.h>
#include <bacnet/basic/object/csv.h>
#include <bacnet/basic/object/device.h>
#include <bacnet/basic/object/piv.h>
#include <bacnet/basic/service/h_apdu.h>
#include <bacnet/datalink/datalink.h>
#include <bacnet/datalink/dlenv.h>
#include <chrono>
#include <cstdint>
#include <cstdio>
#include <iostream>
#include <mutex>
#include <thread>

BacnetServer::BacnetServer(FieldMap &map) : map_(map) {}

BacnetServer::~BacnetServer() { Stop(); }

void BacnetServer::Start() {
    if (IsActive()) {
        std::cout << "Attempted to start BACnet, but already running\n";
        return;
    }

    stop_ = false;

    if (map_.bacnet_device_id)
        Device_Set_Object_Instance_Number(map_.bacnet_device_id.value());

    bacnet_basic_init();
    Device_Object_Name_ANSI_Init(map_.name.c_str());
    DisableWriteServices();
    BuildObjects();
    dlenv_init();

    t_ = std::thread([this]() { this->ServeContext(); });
}

void BacnetServer::ServeContext() {
    using clock = std::chrono::steady_clock;
    auto last_maintenance = clock::now();

    while (!stop_) {
        SetObjectValues();
        bacnet_basic_task();

        auto now = clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(
            now - last_maintenance);
        if (elapsed.count() >= 1) {
            dlenv_maintenance_timer(elapsed.count());
            last_maintenance += elapsed;
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
}

void BacnetServer::DisableWriteServices() {
    apdu_set_confirmed_handler(SERVICE_CONFIRMED_WRITE_PROPERTY, nullptr);
    apdu_set_confirmed_handler(SERVICE_CONFIRMED_WRITE_PROP_MULTIPLE, nullptr);
    apdu_set_confirmed_handler(SERVICE_CONFIRMED_ATOMIC_WRITE_FILE, nullptr);
    apdu_set_confirmed_handler(SERVICE_CONFIRMED_CREATE_OBJECT, nullptr);
    apdu_set_confirmed_handler(SERVICE_CONFIRMED_DELETE_OBJECT, nullptr);
    apdu_set_confirmed_handler(SERVICE_CONFIRMED_REINITIALIZE_DEVICE, nullptr);
    apdu_set_confirmed_handler(SERVICE_CONFIRMED_DEVICE_COMMUNICATION_CONTROL,
                               nullptr);
    apdu_set_unconfirmed_handler(SERVICE_UNCONFIRMED_YOU_ARE, nullptr);
}

void BacnetServer::BuildObjects() {
    for (Field &field : map_.fields) {
        if (!field.bacnet)
            continue;

        switch (field.type) {
        case FieldType::String:
            CharacterString_Value_Create(field.bacnet->instance);
            CharacterString_Value_Name_Set(field.bacnet->instance,
                                           field.name.c_str());
            break;
        case FieldType::Int16:
        case FieldType::Int32:
            Integer_Value_Create(field.bacnet->instance);
            Integer_Value_Name_Set(field.bacnet->instance, field.name.c_str());
            break;
        case FieldType::Uint16:
        case FieldType::Uint32:
            PositiveInteger_Value_Create(field.bacnet->instance);
            PositiveInteger_Value_Name_Set(field.bacnet->instance,
                                           field.name.c_str());
            break;
        case FieldType::Float:
        case FieldType::Double:
            // Doubles will lose precision becuase this library does not
            // implement doubles natively and I don't want to do it
            Analog_Value_Create(field.bacnet->instance);
            Analog_Value_Name_Set(field.bacnet->instance, field.name.c_str());
            break;
        case FieldType::Bool:
            Binary_Value_Create(field.bacnet->instance);
            Binary_Value_Name_Set(field.bacnet->instance, field.name.c_str());
            break;
        }
    }
}

void BacnetServer::SetObjectValues() {
    std::lock_guard lock(*map_.mutex);

    for (Field &field : map_.fields) {
        if (!field.bacnet || !field.bacnet->is_dirty)
            continue;

        switch (field.type) {
        case FieldType::String:
            BACNET_CHARACTER_STRING value;
            char safe[MAX_CHARACTER_STRING_BYTES];
            snprintf(safe, sizeof(safe), "%s",
                     std::get<std::string>(field.value).c_str());
            characterstring_init_ansi(&value, safe);

            CharacterString_Value_Present_Value_Set(field.bacnet->instance,
                                                    &value);
            break;
        case FieldType::Int16:
            Integer_Value_Present_Value_Set(field.bacnet->instance,
                                            std::get<std::int16_t>(field.value),
                                            BACNET_MAX_PRIORITY);
            break;
        case FieldType::Int32:
            Integer_Value_Present_Value_Set(field.bacnet->instance,
                                            std::get<std::int32_t>(field.value),
                                            BACNET_MAX_PRIORITY);
            break;
        case FieldType::Uint16:
            PositiveInteger_Value_Present_Value_Set(
                field.bacnet->instance, std::get<std::uint16_t>(field.value),
                BACNET_MAX_PRIORITY);
            break;
        case FieldType::Uint32:
            PositiveInteger_Value_Present_Value_Set(
                field.bacnet->instance, std::get<std::uint32_t>(field.value),
                BACNET_MAX_PRIORITY);
            break;
        case FieldType::Float:
            Analog_Value_Present_Value_Set(field.bacnet->instance,
                                           std::get<float>(field.value),
                                           BACNET_MAX_PRIORITY);
            break;
        case FieldType::Double:
            Analog_Value_Present_Value_Set(
                field.bacnet->instance,
                static_cast<float>(std::get<double>(field.value)),
                BACNET_MAX_PRIORITY);
            break;
        case FieldType::Bool:
            Binary_Value_Present_Value_Set(
                field.bacnet->instance,
                std::get<bool>(field.value) ? BINARY_ACTIVE : BINARY_INACTIVE);
            break;
        }
        field.bacnet->is_dirty = false;
    }
}

bool BacnetServer::IsActive() { return t_.joinable(); }

void BacnetServer::Stop() {
    stop_ = true;
    if (IsActive())
        t_.join();

    datalink_cleanup();
}
