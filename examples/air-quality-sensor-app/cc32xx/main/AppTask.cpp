/*
 *
 *    Copyright (c) 2020 Project CHIP Authors
 *    Copyright (c) 2020 Texas Instruments Incorporated
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

#include "AppTask.h"
#include "AppConfig.h"
#include "AppEvent.h"

#include <air-quality-sensor-manager.h>
#include <app/server/Server.h>
#include <app/util/attribute-storage.h>

#include "FreeRTOS.h"

#include <credentials/DeviceAttestationCredsProvider.h>
#include <credentials/examples/DeviceAttestationCredsExample.h>
#include <examples/platform/cc32xx/CC32XXDeviceAttestationCreds.h>

#include <CHIPDeviceManager.h>
#include <DeviceCallbacks.h>
#include <lib/support/CHIPMem.h>
#include <lib/support/CHIPPlatformMemory.h>
#include <platform/CHIPDeviceLayer.h>

#include <app/server/Dnssd.h>
#include <app/server/OnboardingCodesUtil.h>

#include "AirQualitySensorAppAttrUpdateDelegate.h"

/* syscfg */
#include <ti_drivers_config.h>

extern "C" {
extern int WiFi_init();
extern void DisplayBanner();
}

#define APP_TASK_STACK_SIZE (4096)
#define APP_TASK_PRIORITY 4
#define APP_EVENT_QUEUE_SIZE 10

#define AIR_QUALITY_SENSOR_ENDPOINT 1

// Added the below three for DNS Server Initialization
using namespace ::chip;
using namespace ::chip::Inet;
using namespace ::chip::System;

using namespace ::chip::Credentials;
using namespace ::chip::DeviceLayer;
using namespace ::chip::DeviceManager;

using namespace chip::app;
using namespace chip::app::Clusters;

static TaskHandle_t sAppTaskHandle;
static QueueHandle_t sAppEventQueue;

AppTask AppTask::sAppTask;

static DeviceCallbacks EchoCallbacks;

int AppTask::StartAppTask()
{
    int ret = 0;

    sAppEventQueue = xQueueCreate(APP_EVENT_QUEUE_SIZE, sizeof(AppEvent));
    if (sAppEventQueue == NULL)
    {
        PLAT_LOG("Failed to allocate app event queue");
        while (true)
            ;
    }

    // Start App task.
    if (xTaskCreate(AppTaskMain, "APP", APP_TASK_STACK_SIZE / sizeof(StackType_t), NULL, APP_TASK_PRIORITY, &sAppTaskHandle) !=
        pdPASS)
    {
        PLAT_LOG("Failed to create app task");
        while (true)
            ;
    }

    // Start App task.
    if (xTaskCreate(AirQualityTaskMain, "AIR-APP", APP_TASK_STACK_SIZE / sizeof(StackType_t), NULL, APP_TASK_PRIORITY,
                    &sAppTaskHandle) != pdPASS)
    {
        PLAT_LOG("Failed to create app task");
        while (true)
            ;
    }
    return ret;
}

int AppTask::Init()
{
    CHIP_ERROR ret;

    cc32xxLogInit();

    /* Initial Terminal, and print Application name */
    DisplayBanner();

    // Init Chip memory management before the stack
    PLAT_LOG("Initialize Memory");
    chip::Platform::MemoryInit();

    PLAT_LOG("Initialize Wi-Fi");
    WiFi_init();

    PLAT_LOG("Initialize CHIP stack");
    ret = PlatformMgr().InitChipStack();
    if (ret != CHIP_NO_ERROR)
    {
        PLAT_LOG("PlatformMgr().InitChipStack() failed");
        while (true)
            ;
    }

    // Init ZCL Data Model and start server
    PLAT_LOG("Initialize Server");
    static chip::CommonCaseDeviceServerInitParams initParams;
    (void) initParams.InitializeStaticResourcesBeforeServerInit();
    chip::Server::GetInstance().Init(initParams);

    // Initialize device attestation config
    PLAT_LOG("Initialize device attestation config");
#ifdef CC32XX_ATTESTATION_CREDENTIALS
    SetDeviceAttestationCredentialsProvider(CC32XX::GetCC32XXDacProvider());
#else

    SetDeviceAttestationCredentialsProvider(Examples::GetExampleDACProvider());
#endif

    ConfigurationMgr().LogDeviceConfig();

    // QR code will be used with CHIP Tool
    PLAT_LOG("Print Onboarding Codes");
    PrintOnboardingCodes(chip::RendezvousInformationFlags(chip::RendezvousInformationFlag::kOnNetwork));

    PLAT_LOG("Start CHIPDeviceManager and Start Event Loop Task");
    CHIPDeviceManager & deviceMgr = CHIPDeviceManager::GetInstance();
    ret                           = deviceMgr.Init(&EchoCallbacks);
    if (ret != CHIP_NO_ERROR)
    {
        PLAT_LOG("CHIPDeviceManager::Init() failed: %s", ErrorStr(ret));
        while (1)
            ;
    }

    AirQualitySensorManager::InitInstance(EndpointId(AIR_QUALITY_SENSOR_ENDPOINT));

    return 0;
}

void AppTask::AppTaskMain(void * pvParameter)
{
    AppEvent event;

    sAppTask.Init();

    while (true)
    {
        /* Task pend until we have stuff to do */
        if (xQueueReceive(sAppEventQueue, &event, portMAX_DELAY) == pdTRUE)
        {
            sAppTask.DispatchEvent(&event);
        }
    }
}

void AppTask::AirQualityTaskMain(void * pvParameter)
{
    AirQualitySensorAppAttrUpdateDelegate runDelegate;
    // this function should never return
}

void AppTask::PostEvent(const AppEvent * aEvent)
{
    if (xQueueSend(sAppEventQueue, aEvent, 0) != pdPASS)
    {
        /* Failed to post the message */
    }
}

void AppTask::DispatchEvent(AppEvent * aEvent)
{
    switch (aEvent->Type)
    {
    case AppEvent::kEventType_AppEvent:
        if (NULL != aEvent->Handler)
        {
            // XXX: assume our caller isn't trying to crash our stack
            aEvent->Handler(aEvent);
        }
        break;

    case AppEvent::kEventType_None:
    default:
        break;
    }
}
