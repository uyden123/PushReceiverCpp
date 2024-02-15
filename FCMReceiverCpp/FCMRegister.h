#pragma once

#include <iostream>
#include <string>
#include <vector>
#include <map>

#include "StringUtil.h"
#include "curl/curl.h"
#include "google/protobuf/util/json_util.h"
#include "android_checkin.pb.h"
#include "checkin.pb.h"
#include "LibCurlWrapper.h"
#include "UtilFunction.h"
#include "json.hpp"

#include "Base64.h"

using json = nlohmann::json;

//Do not change this key!
const std::vector<uint8_t> g_serverKey =
{
    0x04, 0x33, 0x94, 0xf7, 0xdf, 0xa1, 0xeb, 0xb1,
    0xdc, 0x03, 0xa2, 0x5e, 0x15, 0x71, 0xdb, 0x48,
    0xd3, 0x2e, 0xed, 0xed, 0xb2, 0x34, 0xdb, 0xb7,
    0x47, 0x3a, 0x0c, 0x8f, 0xc4, 0xcc, 0xe1, 0x6f,
    0x3c, 0x8c, 0x84, 0xdf, 0xab, 0xb6, 0x66, 0x3e,
    0xf2, 0x0c, 0xd4, 0x8b, 0xfe, 0xe3, 0xf9, 0x76,
    0x2f, 0x14, 0x1c, 0x63, 0x08, 0x6a, 0x6f, 0x2d,
    0xb1, 0x1a, 0x95, 0xb0, 0xce, 0x37, 0xc0, 0x9c,
    0x6e
};


typedef struct _FCM_PARAMS
{
    ECDH_KEYS keys;

    struct Firebase 
    {
        std::string sApiKey;
        std::string sAppID;
        std::string sProjectID;
    } firebase;

    std::string sVapidKey;
} FCM_PARAMS;

typedef struct _FCM_REGISTER_DATA_RETURN
{
    struct ACG 
    {
        std::string sID;
        std::string sSecurityToken;
    } acg;

    struct ECE
    {
		std::string sAuthSecret;
		std::string sPrivateKey;
	} ece;

    std::string sToken;
} FCM_REGISTER_DATA_RETURN;

typedef std::function<void(const std::string&)> LogFnCallback;

class CFCMRegister
{
public:

    CFCMRegister(const LogFnCallback oLogger);
    ~CFCMRegister();

    /**
     * Registers the application with Firebase Cloud Messaging (FCM).
     *
     * This method performs the necessary steps to register the application with FCM.
     * It checks the validity of the provided parameters, performs a check-in request to obtain the Android ID and security token,
     * registers the application with GCM (Google Cloud Messaging) to obtain the GCM registration token,
     * sends a POST request to the Firebase Installations API to register an installation and obtain the authentication token,
     * and finally sends a POST request to the Firebase Cloud Messaging (FCM) Registrations API to register the device and obtain the FCM registration token.
     *
     * @param params The FCM_PARAMS object containing the necessary parameters for registration.
     * @return The FCM_REGISTER_DATA_RETURN object containing the registration data, including the Android ID, security token, and FCM registration token.
     */
    FCM_REGISTER_DATA_RETURN RegisterToFCM(const FCM_PARAMS params);

    /**
     * Sends a check-in request to the FCM server.
     *
     * @param nAndroidId The Android ID.
     * @param nSecurityToken The security token.
     * @return The response from the FCM server.
     */
    checkin_proto::AndroidCheckinResponse CheckIn(uint64_t nAndroidId = 0, uint64_t nSecurityToken = 0);

private:
    /**
     * Generates the check-in request for FCM registration.
     *
     * @param nAndroidId The Android ID.
     * @param nSecurityToken The security token.
     * @return The serialized check-in request.
     */
    std::string GetCheckinRequest(uint64_t nAndroidId = 0, uint64_t nSecurityToken = 0);

    /**
      * Generates the check-in request for FCM registration.
      *
      * @param nAndroidId The Android ID.
      * @param nSecurityToken The security token.
      * @return The serialized check-in request.
      */
    std::string DoGCMRegister(const std::string& appId, uint64_t nAndroidId = 0, uint64_t nSecurityToken = 0);

    /**
      * Generates the check-in request for FCM registration.
      *
      * @param nAndroidId The Android ID.
      * @param nSecurityToken The security token.
      * @return The serialized check-in request.
      */
    std::string PostInstallations(const std::string& sAppId, const std::string& sProjectId, const std::string& sApiKey);

    /**
     * Sends a POST request to the Firebase Cloud Messaging (FCM) Registrations API to register a device.
     *
     * @param sProjectId The project ID.
     * @param sApiKey The API key.
     * @param sVapidKey The VAPID key.
     * @param sAuthSecret The authentication secret.
     * @param sInstallationToken The installation token.
     * @param sPublicKey The public key.
     * @param sGcmToken The GCM registration token.
     * @return The FCM registration token.
     */
    std::string PostFcmRegistrations(
        const std::string& sProjectId,
        const std::string& sApiKey,
        const std::string& sVapidKey,
        const std::string& sAuthSecret,
        const std::string& sInstallationToken,
        const std::string& sPublicKey,
        const std::string& sGcmToken);

    const LogFnCallback m_oLog;
};
