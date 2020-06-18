/*
 *
 *    Copyright (c) 2020 Project CHIP Authors
 *    Copyright (c) 2019-2020 Google LLC.
 *    Copyright (c) 2018 Nest Labs, Inc.
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

/**
 *    @file
 *          Utilities for interacting with multiple file partitions and maps
 *          key-value config calls to the correct partition.
 */

#include <platform/internal/CHIPDeviceLayerInternal.h>
#include <platform/internal/testing/ConfigUnitTest.h>

#include <core/CHIPEncoding.h>
#include <platform/Linux/CHIPLinuxStorage.h>
#include <platform/Linux/PosixConfig.h>
#include <support/CodeUtils.h>

namespace chip {
namespace DeviceLayer {
namespace Internal {

static ChipLinuxStorage gChipLinuxFactoryStorage;
static ChipLinuxStorage gChipLinuxConfigStorage;
static ChipLinuxStorage gChipLinuxCountersStorage;

// *** CAUTION ***: Changing the names or namespaces of these values will *break* existing devices.

// NVS namespaces used to store device configuration information.
const char PosixConfig::kConfigNamespace_ChipFactory[]  = "chip-factory";
const char PosixConfig::kConfigNamespace_ChipConfig[]   = "chip-config";
const char PosixConfig::kConfigNamespace_ChipCounters[] = "chip-counters";

// Keys stored in the Chip-factory namespace
const PosixConfig::Key PosixConfig::kConfigKey_SerialNum           = { kConfigNamespace_ChipFactory, "serial-num" };
const PosixConfig::Key PosixConfig::kConfigKey_MfrDeviceId         = { kConfigNamespace_ChipFactory, "device-id" };
const PosixConfig::Key PosixConfig::kConfigKey_MfrDeviceCert       = { kConfigNamespace_ChipFactory, "device-cert" };
const PosixConfig::Key PosixConfig::kConfigKey_MfrDeviceICACerts   = { kConfigNamespace_ChipFactory, "device-ca-certs" };
const PosixConfig::Key PosixConfig::kConfigKey_MfrDevicePrivateKey = { kConfigNamespace_ChipFactory, "device-key" };
const PosixConfig::Key PosixConfig::kConfigKey_ProductRevision     = { kConfigNamespace_ChipFactory, "product-rev" };
const PosixConfig::Key PosixConfig::kConfigKey_ManufacturingDate   = { kConfigNamespace_ChipFactory, "mfg-date" };
const PosixConfig::Key PosixConfig::kConfigKey_PairingCode         = { kConfigNamespace_ChipFactory, "pairing-code" };

// Keys stored in the Chip-config namespace
const PosixConfig::Key PosixConfig::kConfigKey_FabricId                    = { kConfigNamespace_ChipConfig, "fabric-id" };
const PosixConfig::Key PosixConfig::kConfigKey_ServiceConfig               = { kConfigNamespace_ChipConfig, "service-config" };
const PosixConfig::Key PosixConfig::kConfigKey_PairedAccountId             = { kConfigNamespace_ChipConfig, "account-id" };
const PosixConfig::Key PosixConfig::kConfigKey_ServiceId                   = { kConfigNamespace_ChipConfig, "service-id" };
const PosixConfig::Key PosixConfig::kConfigKey_FabricSecret                = { kConfigNamespace_ChipConfig, "fabric-secret" };
const PosixConfig::Key PosixConfig::kConfigKey_GroupKeyIndex               = { kConfigNamespace_ChipConfig, "group-key-index" };
const PosixConfig::Key PosixConfig::kConfigKey_LastUsedEpochKeyId          = { kConfigNamespace_ChipConfig, "last-ek-id" };
const PosixConfig::Key PosixConfig::kConfigKey_FailSafeArmed               = { kConfigNamespace_ChipConfig, "fail-safe-armed" };
const PosixConfig::Key PosixConfig::kConfigKey_WiFiStationSecType          = { kConfigNamespace_ChipConfig, "sta-sec-type" };
const PosixConfig::Key PosixConfig::kConfigKey_OperationalDeviceId         = { kConfigNamespace_ChipConfig, "op-device-id" };
const PosixConfig::Key PosixConfig::kConfigKey_OperationalDeviceCert       = { kConfigNamespace_ChipConfig, "op-device-cert" };
const PosixConfig::Key PosixConfig::kConfigKey_OperationalDeviceICACerts   = { kConfigNamespace_ChipConfig, "op-device-ca-certs" };
const PosixConfig::Key PosixConfig::kConfigKey_OperationalDevicePrivateKey = { kConfigNamespace_ChipConfig, "op-device-key" };

// Prefix used for NVS keys that contain Chip group encryption keys.
const char PosixConfig::kGroupKeyNamePrefix[] = "gk-";

ChipLinuxStorage * PosixConfig::GetStorageForNamespace(Key key)
{
    if (strcmp(key.Namespace, kConfigNamespace_ChipFactory) == 0)
        return &gChipLinuxFactoryStorage;

    if (strcmp(key.Namespace, kConfigNamespace_ChipConfig) == 0)
        return &gChipLinuxConfigStorage;

    if (strcmp(key.Namespace, kConfigNamespace_ChipCounters) == 0)
        return &gChipLinuxCountersStorage;

    return NULL;
}

CHIP_ERROR PosixConfig::Init()
{
    CHIP_ERROR err = CHIP_NO_ERROR;
    return err;
}

CHIP_ERROR PosixConfig::ReadConfigValue(Key key, bool & val)
{
    CHIP_ERROR err;
    ChipLinuxStorage * storage;
    uint32_t intVal;

    storage = GetStorageForNamespace(key);
    VerifyOrExit(storage != NULL, err = CHIP_ERROR_PERSISTED_STORAGE_FAIL);

    err = storage->ReadValue(key.Name, intVal);
    if (err == CHIP_ERROR_KEY_NOT_FOUND)
    {
        err = CHIP_DEVICE_ERROR_CONFIG_NOT_FOUND;
    }
    SuccessOrExit(err);

    val = (intVal != 0);

exit:
    return err;
}

CHIP_ERROR PosixConfig::ReadConfigValue(Key key, uint32_t & val)
{
    CHIP_ERROR err;
    ChipLinuxStorage * storage;

    storage = GetStorageForNamespace(key);
    VerifyOrExit(storage != NULL, err = CHIP_ERROR_PERSISTED_STORAGE_FAIL);

    err = storage->ReadValue(key.Name, val);
    if (err == CHIP_ERROR_KEY_NOT_FOUND)
    {
        err = CHIP_DEVICE_ERROR_CONFIG_NOT_FOUND;
    }
    SuccessOrExit(err);

exit:
    return err;
}

CHIP_ERROR PosixConfig::ReadConfigValue(Key key, uint64_t & val)
{
    CHIP_ERROR err;
    ChipLinuxStorage * storage;

    storage = GetStorageForNamespace(key);
    VerifyOrExit(storage != NULL, err = CHIP_ERROR_PERSISTED_STORAGE_FAIL);

    // Special case the MfrDeviceId value, optionally allowing it to be read as a blob containing
    // a 64-bit big-endian integer, instead of a u64 value.
    if (key == kConfigKey_MfrDeviceId)
    {
        uint8_t deviceIdBytes[sizeof(uint64_t)];
        size_t deviceIdLen = sizeof(deviceIdBytes);
        size_t deviceIdOutLen;
        err = storage->ReadValueBin(key.Name, deviceIdBytes, deviceIdLen, deviceIdOutLen);
        if (err == CHIP_NO_ERROR)
        {
            VerifyOrExit(deviceIdOutLen == sizeof(deviceIdBytes), err = CHIP_ERROR_INCORRECT_STATE);
            val = Encoding::BigEndian::Get64(deviceIdBytes);
            ExitNow();
        }
    }

    err = storage->ReadValue(key.Name, val);
    if (err == CHIP_ERROR_KEY_NOT_FOUND)
    {
        err = CHIP_DEVICE_ERROR_CONFIG_NOT_FOUND;
    }
    SuccessOrExit(err);

exit:
    return err;
}

CHIP_ERROR PosixConfig::ReadConfigValueStr(Key key, char * buf, size_t bufSize, size_t & outLen)
{
    CHIP_ERROR err;
    ChipLinuxStorage * storage;

    storage = GetStorageForNamespace(key);
    VerifyOrExit(storage != NULL, err = CHIP_ERROR_PERSISTED_STORAGE_FAIL);

    err = storage->ReadValueStr(key.Name, buf, bufSize, outLen);
    if (err == CHIP_ERROR_KEY_NOT_FOUND)
    {
        outLen = 0;
        err    = CHIP_DEVICE_ERROR_CONFIG_NOT_FOUND;
    }
    else if (err == CHIP_ERROR_BUFFER_TOO_SMALL)
    {
        err = (buf == NULL) ? CHIP_NO_ERROR : CHIP_ERROR_BUFFER_TOO_SMALL;
    }
    SuccessOrExit(err);

exit:
    return err;
}

CHIP_ERROR PosixConfig::ReadConfigValueBin(Key key, uint8_t * buf, size_t bufSize, size_t & outLen)
{
    CHIP_ERROR err;
    ChipLinuxStorage * storage;

    storage = GetStorageForNamespace(key);
    VerifyOrExit(storage != NULL, err = CHIP_ERROR_PERSISTED_STORAGE_FAIL);

    err = storage->ReadValueBin(key.Name, buf, bufSize, outLen);
    if (err == CHIP_ERROR_KEY_NOT_FOUND)
    {
        outLen = 0;
        err    = CHIP_DEVICE_ERROR_CONFIG_NOT_FOUND;
    }
    else if (err == CHIP_ERROR_BUFFER_TOO_SMALL)
    {
        err = (buf == NULL) ? CHIP_NO_ERROR : CHIP_ERROR_BUFFER_TOO_SMALL;
    }
    SuccessOrExit(err);

exit:
    return err;
}

CHIP_ERROR PosixConfig::WriteConfigValue(Key key, bool val)
{
    CHIP_ERROR err;
    ChipLinuxStorage * storage;

    storage = GetStorageForNamespace(key);
    VerifyOrExit(storage != NULL, err = CHIP_ERROR_PERSISTED_STORAGE_FAIL);

    err = storage->WriteValue(key.Name, val ? true : false);
    SuccessOrExit(err);

    // Commit the value to the persistent store.
    err = storage->Commit();
    SuccessOrExit(err);

    ChipLogProgress(DeviceLayer, "NVS set: %s/%s = %s", key.Namespace, key.Name, val ? "true" : "false");

exit:
    return err;
}

CHIP_ERROR PosixConfig::WriteConfigValue(Key key, uint32_t val)
{
    CHIP_ERROR err;
    ChipLinuxStorage * storage;

    storage = GetStorageForNamespace(key);
    VerifyOrExit(storage != NULL, err = CHIP_ERROR_PERSISTED_STORAGE_FAIL);

    err = storage->WriteValue(key.Name, val);
    SuccessOrExit(err);

    // Commit the value to the persistent store.
    err = storage->Commit();
    SuccessOrExit(err);

    ChipLogProgress(DeviceLayer, "NVS set: %s/%s = %" PRIu32 " (0x%" PRIX32 ")", key.Namespace, key.Name, val, val);

exit:
    return err;
}

CHIP_ERROR PosixConfig::WriteConfigValue(Key key, uint64_t val)
{
    CHIP_ERROR err;
    ChipLinuxStorage * storage;

    storage = GetStorageForNamespace(key);
    VerifyOrExit(storage != NULL, err = CHIP_ERROR_PERSISTED_STORAGE_FAIL);

    err = storage->WriteValue(key.Name, val);
    SuccessOrExit(err);

    // Commit the value to the persistent store.
    err = storage->Commit();
    SuccessOrExit(err);

    ChipLogProgress(DeviceLayer, "NVS set: %s/%s = %" PRIu64 " (0x%" PRIX64 ")", key.Namespace, key.Name, val, val);

exit:
    return err;
}

CHIP_ERROR PosixConfig::WriteConfigValueStr(Key key, const char * str)
{
    CHIP_ERROR err;
    ChipLinuxStorage * storage;

    if (str != NULL)
    {
        storage = GetStorageForNamespace(key);
        VerifyOrExit(storage != NULL, err = CHIP_ERROR_PERSISTED_STORAGE_FAIL);

        err = storage->WriteValueStr(key.Name, str);
        SuccessOrExit(err);

        // Commit the value to the persistent store.
        err = storage->Commit();
        SuccessOrExit(err);

        ChipLogProgress(DeviceLayer, "NVS set: %s/%s = \"%s\"", key.Namespace, key.Name, str);
    }

    else
    {
        err = ClearConfigValue(key);
        SuccessOrExit(err);
    }

exit:
    return err;
}

CHIP_ERROR PosixConfig::WriteConfigValueStr(Key key, const char * str, size_t strLen)
{
    CHIP_ERROR err;
    char * strCopy = NULL;

    if (str != NULL)
    {
        strCopy = strndup(str, strLen);
        VerifyOrExit(strCopy != NULL, err = CHIP_ERROR_NO_MEMORY);
    }

    err = PosixConfig::WriteConfigValueStr(key, strCopy);

exit:
    if (strCopy != NULL)
    {
        free(strCopy);
    }
    return err;
}

CHIP_ERROR PosixConfig::WriteConfigValueBin(Key key, const uint8_t * data, size_t dataLen)
{
    CHIP_ERROR err;
    ChipLinuxStorage * storage;

    if (data != NULL)
    {
        storage = GetStorageForNamespace(key);
        VerifyOrExit(storage != NULL, err = CHIP_ERROR_PERSISTED_STORAGE_FAIL);

        err = storage->WriteValueBin(key.Name, data, dataLen);
        SuccessOrExit(err);

        // Commit the value to the persistent store.
        err = storage->Commit();
        SuccessOrExit(err);

        ChipLogProgress(DeviceLayer, "NVS set: %s/%s = (blob length %" PRId32 ")", key.Namespace, key.Name, dataLen);
    }
    else
    {
        err = ClearConfigValue(key);
        SuccessOrExit(err);
    }

exit:
    return err;
}

CHIP_ERROR PosixConfig::ClearConfigValue(Key key)
{
    CHIP_ERROR err;
    ChipLinuxStorage * storage;

    storage = GetStorageForNamespace(key);
    VerifyOrExit(storage != NULL, err = CHIP_ERROR_PERSISTED_STORAGE_FAIL);

    err = storage->ClearValue(key.Name);
    if (err == CHIP_ERROR_KEY_NOT_FOUND)
    {
        ExitNow(err = CHIP_NO_ERROR);
    }
    SuccessOrExit(err);

    // Commit the value to the persistent store.
    err = storage->Commit();
    SuccessOrExit(err);

    ChipLogProgress(DeviceLayer, "NVS erase: %s/%s", key.Namespace, key.Name);

exit:
    return err;
}

bool PosixConfig::ConfigValueExists(Key key)
{
    ChipLinuxStorage * storage;

    storage = GetStorageForNamespace(key);
    if (storage == NULL)
        return false;

    return storage->HasValue(key.Name);
}

CHIP_ERROR PosixConfig::EnsureNamespace(const char * ns)
{
    CHIP_ERROR err             = CHIP_NO_ERROR;
    ChipLinuxStorage * storage = NULL;

    if (strcmp(ns, kConfigNamespace_ChipFactory) == 0)
    {
        storage = &gChipLinuxFactoryStorage;
        err     = storage->Init(CHIP_DEFAULT_FACTORY_PATH);
    }
    else if (strcmp(ns, kConfigNamespace_ChipConfig) == 0)
    {
        storage = &gChipLinuxConfigStorage;
        err     = storage->Init(CHIP_DEFAULT_CONFIG_PATH);
    }
    else if (strcmp(ns, kConfigNamespace_ChipCounters) == 0)
    {
        storage = &gChipLinuxCountersStorage;
        err     = storage->Init(CHIP_DEFAULT_DATA_PATH);
    }

    SuccessOrExit(err);

exit:
    return err;
}

CHIP_ERROR PosixConfig::ClearNamespace(const char * ns)
{
    CHIP_ERROR err             = CHIP_NO_ERROR;
    ChipLinuxStorage * storage = NULL;

    if (strcmp(ns, kConfigNamespace_ChipConfig) == 0)
    {
        storage = &gChipLinuxConfigStorage;
    }
    else if (strcmp(ns, kConfigNamespace_ChipCounters) == 0)
    {
        storage = &gChipLinuxCountersStorage;
    }

    VerifyOrExit(storage != NULL, err = CHIP_DEVICE_ERROR_CONFIG_NOT_FOUND);

    err = storage->ClearAll();
    if (err != CHIP_NO_ERROR)
    {
        ChipLogError(DeviceLayer, "Storage ClearAll failed: %s", ErrorStr(err));
    }
    SuccessOrExit(err);

    err = storage->Commit();
    if (err != CHIP_NO_ERROR)
    {
        ChipLogError(DeviceLayer, "Storage Commit failed: %s", ErrorStr(err));
    }

exit:
    return err;
}

CHIP_ERROR PosixConfig::FactoryResetConfig(void)
{
    CHIP_ERROR err = CHIP_NO_ERROR;
    ChipLinuxStorage * storage;

    ChipLogProgress(DeviceLayer, "Performing factory reset");

    storage = &gChipLinuxConfigStorage;
    if (storage == NULL)
    {
        ChipLogError(DeviceLayer, "Storage get failed");
        err = CHIP_DEVICE_ERROR_CONFIG_NOT_FOUND;
    }
    SuccessOrExit(err);

    err = storage->ClearAll();
    if (err != CHIP_NO_ERROR)
    {
        ChipLogError(DeviceLayer, "Storage ClearAll failed: %s", ErrorStr(err));
    }
    SuccessOrExit(err);

    err = storage->Commit();
    if (err != CHIP_NO_ERROR)
    {
        ChipLogError(DeviceLayer, "Storage Commit failed: %s", ErrorStr(err));
    }

exit:
    return err;
}

void PosixConfig::RunConfigUnitTest()
{
    // Run common unit test.
    ::chip::DeviceLayer::Internal::RunConfigUnitTest<PosixConfig>();
}

} // namespace Internal
} // namespace DeviceLayer
} // namespace chip
