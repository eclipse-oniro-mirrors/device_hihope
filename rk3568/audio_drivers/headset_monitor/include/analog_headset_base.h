/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef ANALOG_HEADSET_BASE_H
#define ANALOG_HEADSET_BASE_H

#include "hdf_device_desc.h"

#define INPUT_DEVID_VENDOR      0x0001
#define INPUT_DEVID_PRODUCT     0x0001
#define INPUT_DEVID_VERSION     0x0100

void SetStateSync(unsigned int id, bool state);
int32_t CreateAndRegisterHdfInputDevice(void *hs, struct HdfDeviceObject *device);
void DestroyHdfInputDevice(void);
#endif /* ANALOG_HEADSET_BASE_H */
