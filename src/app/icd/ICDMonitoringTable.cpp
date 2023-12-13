/**
 *
 *    Copyright (c) 2023 Project CHIP Authors
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

#include "ICDMonitoringTable.h"

#include <crypto/RandUtils.h>

namespace chip {

enum class Fields : uint8_t
{
    kCheckInNodeID    = 1,
    kMonitoredSubject = 2,
    kKey              = 3,
};

CHIP_ERROR ICDMonitoringEntry::UpdateKey(StorageKeyName & skey)
{
    VerifyOrReturnError(kUndefinedFabricIndex != this->fabricIndex, CHIP_ERROR_INVALID_FABRIC_INDEX);
    skey = DefaultStorageKeyAllocator::ICDManagementTableEntry(this->fabricIndex, index);
    return CHIP_NO_ERROR;
}

CHIP_ERROR ICDMonitoringEntry::Serialize(TLV::TLVWriter & writer) const
{
    TLV::TLVType outer;
    ReturnErrorOnFailure(writer.StartContainer(TLV::AnonymousTag(), TLV::kTLVType_Structure, outer));
    ReturnErrorOnFailure(writer.Put(TLV::ContextTag(Fields::kCheckInNodeID), checkInNodeID));
    ReturnErrorOnFailure(writer.Put(TLV::ContextTag(Fields::kMonitoredSubject), monitoredSubject));

    ByteSpan buf(key.As<Crypto::Aes128KeyByteArray>());
    ReturnErrorOnFailure(writer.Put(TLV::ContextTag(Fields::kKey), buf));
    ReturnErrorOnFailure(writer.EndContainer(outer));
    return CHIP_NO_ERROR;
}

CHIP_ERROR ICDMonitoringEntry::Deserialize(TLV::TLVReader & reader)
{

    CHIP_ERROR err = CHIP_NO_ERROR;
    TLV::TLVType outer;

    ReturnErrorOnFailure(reader.Next(TLV::AnonymousTag()));
    VerifyOrReturnError(TLV::kTLVType_Structure == reader.GetType(), CHIP_ERROR_WRONG_TLV_TYPE);
    ReturnErrorOnFailure(reader.EnterContainer(outer));
    while ((err = reader.Next()) == CHIP_NO_ERROR)
    {
        if (TLV::IsContextTag(reader.GetTag()))
        {
            switch (TLV::TagNumFromTag(reader.GetTag()))
            {
            case to_underlying(Fields::kCheckInNodeID):
                ReturnErrorOnFailure(reader.Get(checkInNodeID));
                break;
            case to_underlying(Fields::kMonitoredSubject):
                ReturnErrorOnFailure(reader.Get(monitoredSubject));
                break;
            case to_underlying(Fields::kKey): {
                ByteSpan buf(key.AsMutable<Crypto::Aes128KeyByteArray>());
                ReturnErrorOnFailure(reader.Get(buf));
                // Since we are storing either the raw key or a key ID, we must
                // simply copy the data as is in the keyHandle.
                // Calling SetKey here would create another key in storage and will cause
                // key leakage in some implementation.
                memcpy(key.AsMutable<Crypto::Aes128KeyByteArray>(), buf.data(), sizeof(Crypto::Aes128KeyByteArray));
                keyHandleValid = true;
            }
            break;
            default:
                break;
            }
        }
    }

    VerifyOrReturnError(err == CHIP_END_OF_TLV, err);
    ReturnErrorOnFailure(reader.ExitContainer(outer));
    return CHIP_NO_ERROR;
}

void ICDMonitoringEntry::Clear()
{
    this->checkInNodeID    = kUndefinedNodeId;
    this->monitoredSubject = kUndefinedNodeId;
    this->keyHandleValid   = false;
}

CHIP_ERROR ICDMonitoringEntry::SetKey(ByteSpan keyData)
{
    VerifyOrReturnError(keyData.size() == sizeof(Crypto::Aes128KeyByteArray), CHIP_ERROR_INVALID_ARGUMENT);
    VerifyOrReturnError(symmetricKeystore != nullptr, CHIP_ERROR_INTERNAL);
    VerifyOrReturnError(!keyHandleValid, CHIP_ERROR_INTERNAL);

    Crypto::Aes128KeyByteArray keyMaterial;
    memcpy(keyMaterial, keyData.data(), sizeof(Crypto::Aes128KeyByteArray));

    ReturnErrorOnFailure(symmetricKeystore->CreateKey(keyMaterial, key));
    keyHandleValid = true;

    return CHIP_NO_ERROR;
}

CHIP_ERROR ICDMonitoringEntry::DeleteKey()
{
    VerifyOrReturnError(symmetricKeystore != nullptr, CHIP_ERROR_INTERNAL);
    symmetricKeystore->DestroyKey(this->key);
    keyHandleValid = false;
    return CHIP_NO_ERROR;
}

bool ICDMonitoringEntry::IsKeyEquivalent(ByteSpan keyData)
{
    VerifyOrReturnValue(keyData.size() == Crypto::CHIP_CRYPTO_SYMMETRIC_KEY_LENGTH_BYTES, false);
    VerifyOrReturnValue(symmetricKeystore != nullptr, false);
    VerifyOrReturnValue(keyHandleValid, false);

    ICDMonitoringEntry tempEntry(symmetricKeystore);

    VerifyOrReturnValue(tempEntry.SetKey(keyData) == CHIP_NO_ERROR, false);

    // Challenge
    uint8_t mic[Crypto::CHIP_CRYPTO_AEAD_MIC_LENGTH_BYTES]  = { 0 };
    uint8_t aead[Crypto::CHIP_CRYPTO_AEAD_MIC_LENGTH_BYTES] = { 0 };

    CHIP_ERROR err;

    uint64_t data = Crypto::GetRandU64(), validation, encrypted;
    validation    = data;

    err = Crypto::AES_CCM_encrypt(reinterpret_cast<uint8_t *>(&data), sizeof(data), nullptr, 0, tempEntry.key, aead,
                                  Crypto::CHIP_CRYPTO_AEAD_NONCE_LENGTH_BYTES, reinterpret_cast<uint8_t *>(&encrypted), mic,
                                  Crypto::CHIP_CRYPTO_AEAD_MIC_LENGTH_BYTES);

    data = 0;
    if (err == CHIP_NO_ERROR)
    {
        err = Crypto::AES_CCM_decrypt(reinterpret_cast<uint8_t *>(&encrypted), sizeof(encrypted), nullptr, 0, mic,
                                      Crypto::CHIP_CRYPTO_AEAD_MIC_LENGTH_BYTES, key, aead,
                                      Crypto::CHIP_CRYPTO_AEAD_NONCE_LENGTH_BYTES, reinterpret_cast<uint8_t *>(&data));
    }
    tempEntry.DeleteKey();

    if (err != CHIP_NO_ERROR)
    {
        return false;
    }

    return (data == validation) ? true : false;
}

ICDMonitoringEntry & ICDMonitoringEntry::operator=(const ICDMonitoringEntry & icdMonitoringEntry)
{
    if (this == &icdMonitoringEntry)
    {
        return *this;
    }

    fabricIndex       = icdMonitoringEntry.fabricIndex;
    checkInNodeID     = icdMonitoringEntry.checkInNodeID;
    monitoredSubject  = icdMonitoringEntry.monitoredSubject;
    index             = icdMonitoringEntry.index;
    keyHandleValid    = icdMonitoringEntry.keyHandleValid;
    symmetricKeystore = icdMonitoringEntry.symmetricKeystore;
    memcpy(key.AsMutable<Crypto::Aes128KeyByteArray>(), icdMonitoringEntry.key.As<Crypto::Aes128KeyByteArray>(),
           sizeof(Crypto::Aes128KeyByteArray));

    return *this;
}

CHIP_ERROR ICDMonitoringTable::Get(uint16_t index, ICDMonitoringEntry & entry) const
{
    entry.fabricIndex = this->mFabric;
    entry.index       = index;
    ReturnErrorOnFailure(entry.Load(this->mStorage));
    entry.fabricIndex = this->mFabric;
    return CHIP_NO_ERROR;
}

CHIP_ERROR ICDMonitoringTable::Find(NodeId id, ICDMonitoringEntry & entry)
{
    uint16_t index = 0;
    while (index < this->Limit())
    {
        ReturnErrorOnFailure(this->Get(index++, entry));
        if (id == entry.checkInNodeID)
        {
            return CHIP_NO_ERROR;
        }
    }
    entry.index = index;
    return CHIP_ERROR_NOT_FOUND;
}

CHIP_ERROR ICDMonitoringTable::Set(uint16_t index, const ICDMonitoringEntry & entry)
{
    VerifyOrReturnError(index < this->Limit(), CHIP_ERROR_INVALID_ARGUMENT);
    VerifyOrReturnError(kUndefinedNodeId != entry.checkInNodeID, CHIP_ERROR_INVALID_ARGUMENT);
    VerifyOrReturnError(kUndefinedNodeId != entry.monitoredSubject, CHIP_ERROR_INVALID_ARGUMENT);
    VerifyOrReturnError(entry.keyHandleValid, CHIP_ERROR_INVALID_ARGUMENT);
    ICDMonitoringEntry e(this->mFabric, index);
    e.checkInNodeID    = entry.checkInNodeID;
    e.monitoredSubject = entry.monitoredSubject;
    e.index            = index;
    memcpy(e.key.AsMutable<Crypto::Aes128KeyByteArray>(), entry.key.As<Crypto::Aes128KeyByteArray>(),
           sizeof(Crypto::Aes128KeyByteArray));

    return e.Save(this->mStorage);
}

CHIP_ERROR ICDMonitoringTable::Remove(uint16_t index)
{
    ICDMonitoringEntry entry(mSymmetricKeystore, this->mFabric);

    // Retrieve entry and delete the key first as to not
    // cause any key leakage.
    this->Get(index, entry);
    ReturnErrorOnFailure(entry.DeleteKey());

    // Shift remaining entries down one position
    while (CHIP_NO_ERROR == this->Get(static_cast<uint16_t>(index + 1), entry))
    {
        ReturnErrorOnFailure(this->Set(index++, entry));
    }

    // Remove last entry
    entry.fabricIndex = this->mFabric;
    entry.index       = index;

    // entry.Delete() doesn't delete the key from the AES128KeyHandle
    return entry.Delete(this->mStorage);
}

CHIP_ERROR ICDMonitoringTable::RemoveAll()
{
    ICDMonitoringEntry entry(mSymmetricKeystore, this->mFabric);
    uint16_t index = 0;
    while (index < this->Limit())
    {
        CHIP_ERROR err = this->Get(index++, entry);
        if (CHIP_ERROR_NOT_FOUND == err)
        {
            break;
        }
        ReturnErrorOnFailure(err);
        entry.fabricIndex = this->mFabric;
        ReturnErrorOnFailure(entry.DeleteKey());
        ReturnErrorOnFailure(entry.Delete(this->mStorage));
    }
    return CHIP_NO_ERROR;
}

bool ICDMonitoringTable::IsEmpty()
{
    ICDMonitoringEntry entry(mSymmetricKeystore, this->mFabric);
    return (this->Get(0, entry) == CHIP_ERROR_NOT_FOUND);
}

uint16_t ICDMonitoringTable::Limit() const
{
    return mLimit;
}

} // namespace chip
