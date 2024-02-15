#pragma once

#include <vector>
#include <random>
#include <string>

#include "StringUtil.h"
#include "Base64.h"
#include "Http_ece/ece.h"

typedef struct _ECDH_KEYS
{
    std::string sBase64PrivateKey;
    std::string sBase64PublicKey;
    std::string sBase64AuthSecret;
} ECDH_KEYS, *PECDH_KEYS;

namespace UtilFunction
{

    /**
     * Generates a vector of random bytes.
     * 
     * @param nLength The length of the vector to generate.
     * @return A vector of random bytes.
     */
    inline std::vector<uint8_t> GenerateRandomBytes(size_t nLength)
    {
        std::vector<uint8_t> bytes(nLength);
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<> dis(0, 255);
        for (size_t i = 0; i < nLength; ++i) 
        {
            bytes[i] = dis(gen);
        }
        return bytes;
    }

    /**
     * Generates a Firebase FID (Firebase Instance ID).
     * 
     * @return The generated Firebase FID.
     */
    inline std::string GenerateFirebaseFID()
    {
        std::vector<uint8_t> fid = GenerateRandomBytes(17);
        fid[0] = 0b01110000 + (fid[0] % 0b00010000);
        std::string sFirebaseFID = base64_encode(fid.data(), fid.size(), true);
        StringUtil::replace_all(sFirebaseFID, "=", "");

        return sFirebaseFID;
    }

    /**
     * Converts a decimal string to a hexadecimal string.
     * 
     * @param decimalStr The decimal string to convert.
     * @return The hexadecimal string representation of the decimal value.
     */
    inline std::string DecimalToHex(const std::string& decimalStr)
    {
        unsigned long long decimalValue = std::stoull(decimalStr);

        std::stringstream ss;
        ss << std::hex << std::uppercase << decimalValue;
        return ss.str();
    }

    /**
     * Encodes a 32-bit integer value into a varint format and appends it to the given vector.
     * 
     * @param value The 32-bit integer value to encode.
     * @param encoded The vector to append the encoded varint to.
     */
    inline void _EncodeVarint32(uint32_t value, std::vector<uint8_t>& encoded)
    {
        while (value >= 0x80) {
            encoded.emplace_back(static_cast<uint8_t>((value & 0x7F) | 0x80));
            value >>= 7;
        }

        encoded.emplace_back(static_cast<uint8_t>(value));
    }

    /**
     * Decodes a varint-encoded 32-bit integer value from the given data buffer.
     * 
     * @param data The data buffer containing the varint-encoded value.
     * @param dataSize The size of the data buffer.
     * @param offset The offset in the data buffer to start decoding from.
     * @param value [out] The decoded 32-bit integer value.
     * @param varintLen [out] The length of the varint in bytes.
     * @return True if the decoding is successful, false otherwise.
     */
    inline bool _DecodeVarint32(const uint8_t* data, size_t dataSize, size_t offset, uint32_t& value, size_t& varintLen) 
    {
        if (offset >= dataSize)
            return false;

        uint32_t result = 0;
        uint32_t shift = 0;
        uint8_t byte;
        varintLen = 0;

        do 
        {
            if (offset >= dataSize)
                return false;

            byte = data[offset++];
            result |= static_cast<uint32_t>(byte & 0x7F) << shift;
            shift += 7;
            varintLen++;

        } while (byte & 0x80);

        value = result;
        return true;
    }

    /**
     * Generates a pair of ECDH keys.
     * 
     * @return The generated ECDH keys.
     * @throws std::runtime_error if failed to generate the keys.
     */
    inline int GenerateECDHKeys(ECDH_KEYS& keys)
    {
        uint8_t rawRecvPrivKey[ECE_WEBPUSH_PUBLIC_KEY_LENGTH];
        uint8_t rawRecvPubKey[ECE_WEBPUSH_PUBLIC_KEY_LENGTH];
        uint8_t authSecret[ECE_WEBPUSH_AUTH_SECRET_LENGTH];
        
        int nError = ece_webpush_generate_keys(
            rawRecvPrivKey, ECE_WEBPUSH_PRIVATE_KEY_LENGTH,
            rawRecvPubKey, ECE_WEBPUSH_PUBLIC_KEY_LENGTH, 
            authSecret, ECE_WEBPUSH_AUTH_SECRET_LENGTH);

        if (nError != ECE_OK)
            return nError;

        keys.sBase64PrivateKey = base64_encode(rawRecvPrivKey, ECE_WEBPUSH_PRIVATE_KEY_LENGTH, true);
        keys.sBase64PublicKey = base64_encode(rawRecvPubKey, ECE_WEBPUSH_PUBLIC_KEY_LENGTH, true);
        keys.sBase64AuthSecret = base64_encode(authSecret, ECE_WEBPUSH_AUTH_SECRET_LENGTH, true);

        StringUtil::replace_all(keys.sBase64PublicKey, "=", "");
        StringUtil::replace_all(keys.sBase64AuthSecret, "=", "");

        return ECE_OK;
    }
};
