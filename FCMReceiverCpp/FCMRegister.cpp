#include "FcmRegister.h"

CFCMRegister::CFCMRegister(const LogFnCallback oLogger): m_oLog(oLogger){}
CFCMRegister::~CFCMRegister(){}

std::string CFCMRegister::GetCheckinRequest(uint64_t nAndroidId, uint64_t nSecurityToken)
{
    checkin_proto::AndroidCheckinRequest protoAndroidCheckinRequest;

    protoAndroidCheckinRequest.set_user_serial_number(0);
    protoAndroidCheckinRequest.set_version(3);
    protoAndroidCheckinRequest.set_id(nAndroidId);
    protoAndroidCheckinRequest.set_security_token(nSecurityToken);

    checkin_proto::AndroidCheckinProto* checkinPayload = protoAndroidCheckinRequest.mutable_checkin();
    checkinPayload->set_type(checkin_proto::DeviceType::DEVICE_CHROME_BROWSER);

    checkin_proto::ChromeBuildProto* chromeBuild = checkinPayload->mutable_chrome_build();
    chromeBuild->set_platform(checkin_proto::ChromeBuildProto_Platform::ChromeBuildProto_Platform_PLATFORM_MAC);
    chromeBuild->set_chrome_version("87.0.4280.66");
    chromeBuild->set_channel(checkin_proto::ChromeBuildProto_Channel::ChromeBuildProto_Channel_CHANNEL_STABLE);

    std::string sBuffer;
    protoAndroidCheckinRequest.SerializeToString(&sBuffer);

    return sBuffer;
}

checkin_proto::AndroidCheckinResponse CFCMRegister::CheckIn(uint64_t nAndroidId, uint64_t nSecurityToken)
{
    std::string sCheckinUrl = "https://android.clients.google.com/checkin";
    std::string sBuffer = GetCheckinRequest(nAndroidId, nSecurityToken);
    std::string sResponseBuffer;

    CLibCurlWrapper cLibCurlWrapper;

    std::vector<std::string> headers;
    headers.reserve(1);
    headers.emplace_back("Content-Type: application/x-protobuf");

    cLibCurlWrapper.SetHeaders(headers);
    if (!cLibCurlWrapper.Post(sCheckinUrl, sBuffer, sResponseBuffer))
    {
        m_oLog("[FCMRegister][Error] CheckIn: " + cLibCurlWrapper.GetError());
        return checkin_proto::AndroidCheckinResponse();
    }

    checkin_proto::AndroidCheckinResponse protoCheckinResponse;
    if (!protoCheckinResponse.ParseFromString(sResponseBuffer))
    {
        m_oLog("[FCMRegister][Error] CheckIn: Failed to parse checkin response.");
        return checkin_proto::AndroidCheckinResponse();
    }

    return protoCheckinResponse;
}

std::string CFCMRegister::DoGCMRegister(const std::string& sAppId, uint64_t sAndroidId, uint64_t sSecurityToken)
{
    std::string sServerKeyBase64Encoded = base64_encode(g_serverKey.data(), g_serverKey.size(), true);
    StringUtil::replace_all(sServerKeyBase64Encoded, "=", "");

    std::string sHeader = "Authorization: AidLogin ";
    sHeader.append(StringUtil::to_string(sAndroidId)).append(":").append(StringUtil::to_string(sSecurityToken));

    std::string sBody;
    sBody.append("app=org.chromium.linux&X-subtype=").append(sAppId)
        .append("&device=").append(std::to_string(sAndroidId))
        .append("&sender=").append(sServerKeyBase64Encoded);

    std::string sResponseBuffer;
    std::string sRegisterUrl = "https://android.clients.google.com/c2dm/register3";

    CLibCurlWrapper cLibCurlWrapper;

    std::vector<std::string> headers;
    headers.emplace_back(std::move(sHeader));
    cLibCurlWrapper.SetHeaders(headers);

    if (!cLibCurlWrapper.Post(sRegisterUrl, sBody, sResponseBuffer))
    {
        m_oLog("[FCMRegister][Error] DoGCMRegister: Error:" + cLibCurlWrapper.GetError());
        return std::string();
    }

    return StringUtil::split(sResponseBuffer, "=").at(1);
}

std::string CFCMRegister::PostInstallations(const std::string& sAppId, const std::string& sProjectId, const std::string& sApiKey)
{
    std::string sBody;
    sBody.append("{\"appId\":\"").append(sAppId)
        .append("\",\"authVersion\":\"FIS_v2\",\"sdkVersion\":\"w:0.6.4\",\"fid\":\"")
        .append(UtilFunction::GenerateFirebaseFID()).append("\"}");

    std::string sResponseBuffer;
    std::string sUrl = "https://firebaseinstallations.googleapis.com/v1/projects/";
    sUrl.append(sProjectId).append("/installations");

    CLibCurlWrapper cLibCurlWrapper;

    std::string sHeartbeat = "{\"heartbeats\":[],\"version\":2}";
    std::string sClient = base64_encode(sHeartbeat, true);

    std::vector<std::string> sHeaders;
    sHeaders.emplace_back("x-firebase-client: " + sClient);
    sHeaders.emplace_back("x-goog-api-key: " + sApiKey);
    sHeaders.emplace_back("Content-Type: application/json");

    cLibCurlWrapper.SetHeaders(sHeaders);

    if (!cLibCurlWrapper.Post(sUrl, sBody, sResponseBuffer))
    {
        m_oLog("[FCMRegister][Error] PostInstallations: Error:" + cLibCurlWrapper.GetError());
        return std::string();
    }

    json jsonResponse;
    try
    {
        jsonResponse = json::parse(sResponseBuffer);
    }
    catch (const std::exception& e)
    {
        m_oLog("[FCMRegister][Error] PostInstallations: Failed to parse JSON response:" + std::string(e.what()));
        return std::string();
    }

    if (!jsonResponse.contains("authToken") || !jsonResponse["authToken"].contains("token"))
    {
        m_oLog("[FCMRegister][Error] PostInstallations: Invalid JSON response format.");
        return std::string();
    }

    return jsonResponse["authToken"]["token"];
}

std::string CFCMRegister::PostFcmRegistrations(const std::string& sProjectId, const std::string& sApiKey, const std::string& sVapidKey, const std::string& sAuthSecret, const std::string& sInstallationToken, const std::string& sPublicKey, const std::string& sGcmToken)
{
    std::string sBody;
    sBody.append("{\"web\":{\"applicationPubKey\":\"").append(sVapidKey)
        .append("\",\"auth\":\"").append(sAuthSecret)
        .append("\",\"endpoint\":\"https://fcm.googleapis.com/fcm/send/")
        .append(sGcmToken).append("\",\"p256dh\":\"").append(sPublicKey).append("\"}}");

    std::string sResponseBuffer;
    std::string sUrl = "https://fcmregistrations.googleapis.com/v1/projects/";
    sUrl.append(sProjectId).append("/registrations");

    CLibCurlWrapper cLibCurlWrapper;

    std::vector<std::string> headers;
    headers.emplace_back("Content-Type: application/json");
    headers.emplace_back("x-goog-api-key: " + sApiKey);
    headers.emplace_back("x-goog-firebase-installations-auth: " + sInstallationToken);

    cLibCurlWrapper.SetHeaders(headers);
    if (!cLibCurlWrapper.Post(sUrl, sBody, sResponseBuffer))
    {
        m_oLog("[FCMRegister][Error] PostFcmRegistrations: Error: " + cLibCurlWrapper.GetError());
        return std::string();
    }

    json jsonResponse;
    try
    {
        jsonResponse = json::parse(sResponseBuffer);
    }
    catch (const std::exception& e)
    {
        m_oLog("[FCMRegister][Error] PostFcmRegistrations: Failed to parse JSON response:" + std::string(e.what()));
        return std::string();
    }

    if (!jsonResponse.contains("token"))
    {
        m_oLog("[FCMRegister][Error] PostFcmRegistrations: Invalid JSON response format.");
        return std::string();
    }

    return jsonResponse["token"];
}

FCM_REGISTER_DATA_RETURN CFCMRegister::RegisterToFCM(const FCM_PARAMS params)
{
    FCM_REGISTER_DATA_RETURN fcmRegisterData;
    if (params.keys.sBase64AuthSecret.empty() || params.keys.sBase64PublicKey.empty() || params.keys.sBase64PrivateKey.empty() ||
        params.firebase.sApiKey.empty() || params.firebase.sAppID.empty() || params.firebase.sProjectID.empty() ||
        params.sVapidKey.empty())
    {
        m_oLog("[FCMRegister][Error] RegisterToFCM: Invalid params. Please provide all required fields.");
        return fcmRegisterData;
    }

    checkin_proto::AndroidCheckinResponse checkin = CFCMRegister::CheckIn();

    if (checkin.android_id() == 0 || checkin.security_token() == 0)
    {
        m_oLog("[FCMRegister][Error] RegisterToFCM: CheckIn error");
        return fcmRegisterData;
    }

    std::string sGcmToken = CFCMRegister::DoGCMRegister(params.firebase.sAppID, checkin.android_id(), checkin.security_token());
    if (sGcmToken.empty())
    {
        m_oLog("[FCMRegister][Error] RegisterToFCM: Gcm register error");
        return fcmRegisterData;
    }

    std::string sInstallationToken = CFCMRegister::PostInstallations(params.firebase.sAppID, params.firebase.sProjectID, params.firebase.sApiKey);
    if (sInstallationToken.empty())
    {
        m_oLog("[FCMRegister][Error] RegisterToFCM: Post installations error");
        return fcmRegisterData;
    }

    std::string sFcmToken = CFCMRegister::PostFcmRegistrations(
        params.firebase.sProjectID, 
        params.firebase.sApiKey,
        params.sVapidKey,
        params.keys.sBase64AuthSecret,
        sInstallationToken,
        params.keys.sBase64PublicKey,
        sGcmToken
    );

    if (sFcmToken.empty())
    {
        m_oLog("[FCMRegister][Error] RegisterToFCM: Post FCM Registration error");
        return fcmRegisterData;
    }

    fcmRegisterData.acg.sID = StringUtil::to_string(checkin.android_id());
    fcmRegisterData.acg.sSecurityToken = StringUtil::to_string(checkin.security_token());
    fcmRegisterData.ece.sAuthSecret = params.keys.sBase64AuthSecret;
    fcmRegisterData.ece.sPrivateKey = params.keys.sBase64PrivateKey;
    fcmRegisterData.sToken = sFcmToken;

    return fcmRegisterData;
}
