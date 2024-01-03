/*
 *
 *    Copyright (c) 2022 Project CHIP Authors
 *    All rights reserved.
 *
 *    Licensed under the Apache License, Version 2.0 (the "License");
 *    you may not use this file except in compliance with the License.
 *    You may obtain a copy of the License at
 *
 *        http://www.apache.org/licenses/LICENSE-2.0
 *
 *    Unless required by applicable law or agreed to in writing, software
 *    distributed under the License is distributed on an "AS IS" BASIS,
 *    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *    See the License for the specific language governing permissions and
 *    limitations under the License.
 */

#include "EvseAppCommandDelegate.h"
#include <lib/support/BytesToHex.h>
#include <platform/PlatformManager.h>

#include <EvseManager.h>
#include <utility>

using chip::to_underlying;

class EvseAppCommandHandler
{
public:
    static EvseAppCommandHandler * FromJSON(const char * json);

    static void HandleCommand(intptr_t context);

    EvseAppCommandHandler(std::string && cmd, Json::Value && params) :
        mCommandName(std::move(cmd)), mCommandParameters(std::move(params))
    {}

private:
    // aCommand should be "lock" or "unlock".
    static CHIP_ERROR ExtractPINFromParams(const char * aCommand, const Json::Value & aParams, Optional<chip::ByteSpan> & aPIN,
                                           chip::Platform::ScopedMemoryBuffer<uint8_t> & aPINBuffer);

    std::string mCommandName;
    Json::Value mCommandParameters;
};

EvseAppCommandHandler * EvseAppCommandHandler::FromJSON(const char * json)
{
    // Command format:
    // { "Cmd": "SetDoorState", "Params": { "EndpointId": 1, "DoorState": 2} }
    Json::Reader reader;
    Json::Value value;
    if (!reader.parse(json, value))
    {
        ChipLogError(NotSpecified, "EVSE App: Error parsing JSON with error %s:", reader.getFormattedErrorMessages().c_str());
        return nullptr;
    }

    if (value.empty() || !value.isObject())
    {
        ChipLogError(NotSpecified, "EVSE App: Invalid JSON command received");
        return nullptr;
    }

    if (!value.isMember("Cmd") || !value["Cmd"].isString())
    {
        ChipLogError(NotSpecified, "EVSE App: Invalid JSON command received: command name is missing");
        return nullptr;
    }

    Json::Value params = Json::objectValue;
    if (value.isMember("Params"))
    {
        if (!value["Params"].isObject())
        {
            ChipLogError(NotSpecified, "EVSE App: Invalid JSON command received: specified parameters are incorrect");
            return nullptr;
        }
        params = value["Params"];
    }
    auto commandName = value["Cmd"].asString();
    return chip::Platform::New<EvseAppCommandHandler>(std::move(commandName), std::move(params));
}

void EvseAppCommandHandler::HandleCommand(intptr_t context)
{
    auto * self         = reinterpret_cast<EvseAppCommandHandler *>(context);
    const auto & params = self->mCommandParameters;
    // Determine the endpoint ID from the parameters JSON. If it is missing, use the default endpoint defined in the
    // door-lock-server.h
    CHIP_ERROR err              = CHIP_NO_ERROR;
    chip::EndpointId endpointId = DOOR_LOCK_SERVER_ENDPOINT;
    if (params.isMember("EndpointId"))
    {
        VerifyOrExit(params["EndpointId"].isUInt(),
                     ChipLogError(NotSpecified, "EVSE App: Unable to execute command \"%s\": invalid endpoint Id",
                                  self->mCommandName.c_str()));
        endpointId = static_cast<chip::EndpointId>(params["EndpointId"].asUInt());
    }

    // TODO: Make commands separate objects derived from some base class to clean up this mess.

    // Now we can try to execute a command
    if (self->mCommandName == "SetDoorState")
    {
        VerifyOrExit(params.isMember("DoorState"),
                     ChipLogError(NotSpecified,
                                  "EVSE App: Unable to execute command to set the door state: DoorState is missing in command"));

        VerifyOrExit(
            params["DoorState"].isUInt(),
            ChipLogError(NotSpecified, "EVSE App: Unable to execute command to set the door state: invalid type for DoorState"));

        auto doorState = params["DoorState"].asUInt();
        VerifyOrExit(doorState < to_underlying(DoorStateEnum::kUnknownEnumValue),
                     ChipLogError(NotSpecified,
                                  "EVSE App: Unable to execute command to set door state: DoorState is out of range [doorState=%u]",
                                  doorState));
        // EvseManager::Instance().SetDoorState(endpointId, static_cast<DoorStateEnum>(doorState));
    }
    // else if (self->mCommandName == "SendDoorLockAlarm")
    else
    {
        aPIN.ClearValue();
    }

    return CHIP_NO_ERROR;
}

void EvseAppCommandDelegate::OnEventCommandReceived(const char * json)
{
    auto handler = EvseAppCommandHandler::FromJSON(json);
    if (nullptr == handler)
    {
        ChipLogError(NotSpecified, "EVSE App: Unable to instantiate a command handler");
        return;
    }

    chip::DeviceLayer::PlatformMgr().ScheduleWork(EvseAppCommandHandler::HandleCommand, reinterpret_cast<intptr_t>(handler));
}
