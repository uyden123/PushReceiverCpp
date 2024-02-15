#include "FCMClient.h"

CFCMClient::CFCMClient(
	const LogFnCallback oLogger, 
	const std::string sAndroidID, 
	std::string sSecurityToken, 
	std::string sBase64PrivateKey, 
	std::string sBase64AuthSecret,
	std::vector<std::string> persistentIDs) :
		m_PersistentIds(persistentIDs),
		m_oLogger(oLogger),
		m_sAndroidId(sAndroidID),
		m_sSecurityToken(sSecurityToken),
		m_nState(MCS_VERSION_TAG_AND_SIZE),
		m_nSizePacketSoFar(0),
		m_nMessageTag(0),
		m_nMessageSize(0),
		m_nMinBytesNeeded(1)
{
	m_SecureTCPClient = std::make_unique<CTCPSSLClient>(oLogger);

	std::string sDecodedPrivatekey = base64_decode(sBase64PrivateKey, true);
	std::string sDecodedAuth = base64_decode(sBase64AuthSecret, true);

	if (sDecodedPrivatekey.size() != ECE_WEBPUSH_PRIVATE_KEY_LENGTH || sDecodedAuth.size() != ECE_WEBPUSH_AUTH_SECRET_LENGTH)
	{
		std::string sError = "[CFCMClient][FATAL] Invalid private key or auth secret";
		if (bVerbose) m_oLogger(sError);
		throw std::runtime_error("Invalid private key or auth secret");
	}

	m_RawSubPrivKey.insert(m_RawSubPrivKey.begin(), sDecodedPrivatekey.begin(), sDecodedPrivatekey.end());
	m_AuthSecret.insert(m_AuthSecret.begin(), sDecodedAuth.begin(), sDecodedAuth.end());
}

CFCMClient::~CFCMClient() {}

bool CFCMClient::ConnectToServer()
{
	CFCMRegister cFcmRegister(m_oLogger);
	std::uint64_t nAndroidId = 0;
	std::uint64_t nSecurityToken = 0;

	try 
	{
		nAndroidId = std::stoull(m_sAndroidId);
		nSecurityToken = std::stoull(m_sSecurityToken);
	}
	catch (std::exception& e)
	{
		if (bVerbose) m_oLogger("[CFCMClient][FATAL] Invalid Android ID or Security Token");
		return false;
	}

	checkin_proto::AndroidCheckinResponse checkin = cFcmRegister.CheckIn(nAndroidId, nSecurityToken);
	if (checkin.android_id() == 0 || checkin.security_token() == 0)
	{
		if (bVerbose) m_oLogger("[CFCMClient][ERROR] Connect to server error: CheckIn failed");
	}

	if (!m_SecureTCPClient->Connect(m_szHost, m_szPort))
	{
		std::string sError = "[CFCMClient][FATAL] Unable to connect to server";
		if (bVerbose) m_oLogger(sError);
		return false;
	}

	Emit("connected", "[CFCMClient][INFO] Connected to server");
	SendLoginBuffer();

	return true;
}

void CFCMClient::SendLoginBuffer()
{
	std::string sAndroidIdHex = UtilFunction::DecimalToHex(m_sAndroidId);

	mcs_proto::LoginRequest cLoginRequest;
	cLoginRequest.set_adaptive_heartbeat(false);
	cLoginRequest.set_auth_service(mcs_proto::LoginRequest_AuthService::LoginRequest_AuthService_ANDROID_ID);
	cLoginRequest.set_auth_token(m_sSecurityToken.c_str());
	cLoginRequest.set_id("chrome-87.0.4280.66");
	cLoginRequest.set_domain("mcs.android.com");
	cLoginRequest.set_device_id("android-" + sAndroidIdHex);
	cLoginRequest.set_network_type(1);
	cLoginRequest.set_resource(m_sAndroidId.c_str());
	cLoginRequest.set_user(m_sAndroidId.c_str());
	cLoginRequest.set_use_rmq2(true);
	mcs_proto::Setting* setting = cLoginRequest.add_setting();
	setting->set_name("new_vc");
	setting->set_value("1");

	for (const std::string& sPersistentId : m_PersistentIds)
		if(!sPersistentId.empty())
			cLoginRequest.add_received_persistent_id(sPersistentId.c_str());

	std::string sSerialized;
	cLoginRequest.SerializeToString(&sSerialized);

	std::vector<std::uint8_t> buf;
	buf.push_back(kMCSVersion);
	buf.push_back(kLoginRequestTag);

	size_t nSize = sSerialized.size();
	UtilFunction::_EncodeVarint32(nSize, buf);
	buf.insert(buf.end(), sSerialized.begin(), sSerialized.end());

	int nBytesSent = m_SecureTCPClient->Send(reinterpret_cast<const char*>(buf.data()), buf.size());
	if (nBytesSent == SOCKET_ERROR)
	{
		std::string sError = "[CFCMClient][FATAL] Send login request failed with error " + WSAGetLastError();
		if (bVerbose) m_oLogger(sError);
		throw std::runtime_error("Send login request failed");
	}
}

void CFCMClient::SendHeartbeat(int32_t nLastStreamIDReceived)
{
	mcs_proto::HeartbeatPing cHeartbeatPing;
	cHeartbeatPing.set_status(0);
	cHeartbeatPing.set_stream_id(0);
	cHeartbeatPing.set_last_stream_id_received(nLastStreamIDReceived);

	std::string sSerialized;
	cHeartbeatPing.SerializeToString(&sSerialized);

	std::vector<std::uint8_t> buf;
	buf.push_back(kHeartbeatPingTag);

	size_t nSize = sSerialized.size();
	UtilFunction::_EncodeVarint32(nSize, buf);
	buf.insert(buf.end(), sSerialized.begin(), sSerialized.end());

	int nBytesSent = m_SecureTCPClient->Send(reinterpret_cast<const char*>(buf.data()), buf.size());
	if (nBytesSent == SOCKET_ERROR)
	{
		if (bVerbose) m_oLogger("[CFCMClient][ERROR] Send heartbeat failed with error " + WSAGetLastError());
	}

	if (bVerbose) m_oLogger("[CFCMClient][INFO] Sent heartbeat to server");
}

void CFCMClient::StartReceiver()
{
	if (!m_SecureTCPClient->IsConnected())
	{
		std::string sError = "[CFCMClient][FATAL] StartReceiver: It seems like you haven't called ConnectToServer() yet.";
		if (bVerbose) m_oLogger(sError);
		throw std::runtime_error("Not connected to server");
	}

	while (true)
	{
		m_nMinBytesNeeded = CalculateMinBytesNeeded();
		if (m_BytesReadFromServer.size() < m_nMinBytesNeeded)
		{
			std::vector<uint8_t> buf(m_nMinBytesNeeded - m_BytesReadFromServer.size());
			int nBytesRead = m_SecureTCPClient->Receive(reinterpret_cast<char*>(buf.data()), buf.size());
			if (nBytesRead == SOCKET_ERROR)
			{
				if (bVerbose) m_oLogger("[CFCMClient][ERROR] StartReceiver: Receive failed with error " + WSAGetLastError());
				break;
			}
			else
			if (nBytesRead > 0)
			{
				m_BytesReadFromServer.insert(m_BytesReadFromServer.end(), buf.begin(), buf.begin() + nBytesRead);
				if (bVerbose) m_oLogger("[CFCMClient][INFO] Got data size " + std::to_string(m_BytesReadFromServer.size()) + " need " + std::to_string(m_nMinBytesNeeded));

				if (m_BytesReadFromServer.size() >= m_nMinBytesNeeded)
				{
					ProcessData();
				}
			}
		}
	}
}

int CFCMClient::CalculateMinBytesNeeded()
{
	int nMinBytesNeeded = 0;
	switch (m_nState)
	{
	case MCS_VERSION_TAG_AND_SIZE:
		nMinBytesNeeded = kVersionPacketLen + kTagPacketLen + kSizePacketLenMin;
		break;
	case MCS_TAG_AND_SIZE:
		nMinBytesNeeded = kTagPacketLen + kSizePacketLenMin;
		break;
	case MCS_SIZE:
		nMinBytesNeeded = m_nSizePacketSoFar + 1;
		break;
	case MCS_PROTO_BYTES:
		nMinBytesNeeded = m_nMessageSize;
		break;
	default:
	{
		std::string sError = "[CFCMClient][FATAL] CalculateMinBytesNeeded: Unexpected State " + std::to_string(m_nState);
		if (bVerbose) m_oLogger(sError);
		throw std::runtime_error("Unexpected State " + std::to_string(m_nState));
	}
	break;
	}

	return nMinBytesNeeded;
}

void CFCMClient::ProcessData()
{
	switch (m_nState)
	{
	case MCS_VERSION_TAG_AND_SIZE:
		GotVersion();
		break;
	case MCS_TAG_AND_SIZE:
		GotMessageTag();
		break;
	case MCS_SIZE:
		GotMessageSize();
		break;
	case MCS_PROTO_BYTES:
		GotMessageBytes();
		break;
	default:
	{
		std::string sError = "[CFCMClient][FATAL] ProcessData: Unexpected State " + std::to_string(m_nState);
		if (bVerbose) m_oLogger(sError);
		throw std::runtime_error("Unexpected State " + std::to_string(m_nState));
	}
	break;
	}
}

void CFCMClient::GotVersion()
{
	m_nState = MCS_VERSION_TAG_AND_SIZE;
	if (m_BytesReadFromServer.size() < 1)
	{
		if (bVerbose) m_oLogger("[CFCMClient][WARNING] WaitForData: DataSize is too small");
		return;
	}
	int nVersion = m_BytesReadFromServer[0];
	m_BytesReadFromServer.erase(m_BytesReadFromServer.begin());

	if (bVerbose) m_oLogger("[CFCMClient][INFO] Got version " + std::to_string(nVersion));

	if (nVersion < kMCSVersion && nVersion != 38)
	{
		std::string sError = "[CFCMClient][FATAL] Got version " + std::to_string(nVersion) + " but expected " + std::to_string(kMCSVersion);
		if (bVerbose) m_oLogger(sError);
		throw std::runtime_error("Got version " + std::to_string(nVersion) + " but expected " + std::to_string(kMCSVersion));
	}

	m_nState = MCS_TAG_AND_SIZE;
	GotMessageTag();
}

void CFCMClient::GotMessageTag()
{
	if (m_BytesReadFromServer.size() < 1)
	{
		if (bVerbose) m_oLogger("[CFCMClient][WARNING] GotMessageTag: DataSize is too small");
		return;
	}
	m_nMessageTag = m_BytesReadFromServer[0];
	if (bVerbose) m_oLogger("[CFCMClient][INFO] Got message tag " + std::to_string(m_nMessageTag));
	m_BytesReadFromServer.erase(m_BytesReadFromServer.begin());

	m_nState = MCS_SIZE;
	GotMessageSize();
}

void CFCMClient::GotMessageSize()
{
	size_t nVarintlen;
	if (!UtilFunction::_DecodeVarint32(m_BytesReadFromServer.data(), m_BytesReadFromServer.size(), 0, m_nMessageSize, nVarintlen))
	{
		m_nSizePacketSoFar = 1 + m_BytesReadFromServer.size();
		return;
	}
	m_BytesReadFromServer.erase(m_BytesReadFromServer.begin(), m_BytesReadFromServer.begin() + nVarintlen);
	m_nSizePacketSoFar = 0;

	if (bVerbose) m_oLogger("[CFCMClient][INFO] Got message size " + std::to_string(m_nMessageSize));

	m_nState = MCS_PROTO_BYTES;
	if (m_nMessageSize > 0)
	{
		return;
	}

	GotMessageBytes();
}

void CFCMClient::GotMessageBytes()
{
	switch (m_nMessageTag)
	{
	case MCSProtoTag::kLoginResponseTag:
		HandleLoginResponseTag();
		break;
	case MCSProtoTag::kIqStanzaTag:
		HandleIqStanzaTag();
		break;
	case MCSProtoTag::kDataMessageStanzaTag:
		HandleDataMessageStanzaTag();
		break;
	case MCSProtoTag::kHeartbeatAckTag:
		HandleHeartbeatAck();
		break;
	default:
		break;
	}
	m_BytesReadFromServer.clear();
	GetNextMessage();
}

void CFCMClient::HandleLoginResponseTag()
{
	mcs_proto::LoginResponse cLoginResponse;
	if (!cLoginResponse.ParseFromArray(m_BytesReadFromServer.data(), m_BytesReadFromServer.size()))
	{
		if (bVerbose) m_oLogger("[CFCMClient][FATAL] HandleLoginResponseTag: Cannot parse LoginResponse");
		throw std::runtime_error("Cannot parse LoginResponse");
	}
	if (bVerbose) m_oLogger("[CFCMClient][INFO] Got kLoginResponseTag: " + std::to_string(cLoginResponse.last_stream_id_received()));

	m_PersistentIds.clear();
	SendHeartbeat(cLoginResponse.last_stream_id_received());
}
void CFCMClient::HandleIqStanzaTag()
{
	if (bVerbose) m_oLogger("[CFCMClient][INFO] Got kIqStanzaTag");
}

void CFCMClient::HandleDataMessageStanzaTag()
{
	mcs_proto::DataMessageStanza cDataMessageStanza;
	if (!cDataMessageStanza.ParseFromArray(m_BytesReadFromServer.data(), m_BytesReadFromServer.size()))
	{
		if (bVerbose) m_oLogger("[CFCMClient][ERROR] HandleDataMessageStanzaTag: Cannot parse DataMessageStanza");
		return;
	}

	std::string sRawData = cDataMessageStanza.raw_data();

	std::vector<uint8_t> salt[ECE_SALT_LENGTH];
	std::vector<uint8_t> rawSenderPubKey[ECE_WEBPUSH_PUBLIC_KEY_LENGTH];
	for (const mcs_proto::AppData& app : cDataMessageStanza.app_data())
	{
		if (app.key() == "encryption")
		{
			std::string sBase64Salt = StringUtil::split(app.value(), "=").at(1);
			std::string sBase64SaltDecoded = base64_decode(sBase64Salt, true);

			if (sBase64SaltDecoded.size() != ECE_SALT_LENGTH)
			{
				if (bVerbose) m_oLogger("[CFCMClient][ERROR] HandleDataMessageStanzaTag: Invalid salt length");
				return;
			}

			salt->insert(salt->begin(), sBase64SaltDecoded.begin(), sBase64SaltDecoded.end());
		}
		else if (app.key() == "crypto-key")
		{
			std::string sBase64SenderPubKey = StringUtil::split(app.value(), "=").at(1);
			std::string sBase64SenderPubKeyDecoded = base64_decode(sBase64SenderPubKey, true);

			if (sBase64SenderPubKeyDecoded.size() != ECE_WEBPUSH_PUBLIC_KEY_LENGTH)
			{
				if (bVerbose) m_oLogger("[CFCMClient][ERROR] HandleDataMessageStanzaTag: Invalid sender public key length");
				return;
			}

			rawSenderPubKey->insert(rawSenderPubKey->begin(), sBase64SenderPubKeyDecoded.begin(), sBase64SenderPubKeyDecoded.end());
		}
	}

	std::string nPersistentID = cDataMessageStanza.persistent_id();
	if (!nPersistentID.empty())
	{
		m_PersistentIds.emplace_back(nPersistentID);
		Emit("persistent_id", StringUtil::join(m_PersistentIds, ";"));
	}

	if (salt->empty() || rawSenderPubKey->empty() || sRawData.empty() || sRawData.size() < 1)
	{
		if (bVerbose) m_oLogger("[CFCMClient][ERROR] HandleDataMessageStanzaTag: Invalid DataMessageStanza");
		return;
	}

	std::vector<uint8_t> ciphertext(sRawData.begin(), sRawData.end());
	size_t nCiphertextLen = ciphertext.size();

	size_t nPlaintextLen = ece_aesgcm_plaintext_max_length(RS_LENGTH, nCiphertextLen);
	if (nPlaintextLen == 0)
	{
		if (bVerbose) m_oLogger("[CFCMClient][ERROR] HandleDataMessageStanzaTag: Invalid plaintext length");
		return;
	}

	std::vector<uint8_t> plainText(nPlaintextLen);

	int nErrorCode = ece_webpush_aesgcm_decrypt(
		m_RawSubPrivKey.data(), ECE_WEBPUSH_PRIVATE_KEY_LENGTH, m_AuthSecret.data(),
		ECE_WEBPUSH_AUTH_SECRET_LENGTH, salt->data(), ECE_SALT_LENGTH, rawSenderPubKey->data(),
		ECE_WEBPUSH_PUBLIC_KEY_LENGTH, RS_LENGTH, ciphertext.data(), nCiphertextLen, plainText.data(),
		&nPlaintextLen);

	if (nErrorCode != ECE_OK)
	{
		if (bVerbose) m_oLogger("[CFCMClient][ERROR] HandleDataMessageStanzaTag: Decrypt failed with error code " + std::to_string(nErrorCode));
		return;
	}

	std::string sPlainText(reinterpret_cast<char*>(plainText.data()), nPlaintextLen);
	Emit("message", sPlainText);
}

void CFCMClient::HandleHeartbeatAck()
{
	mcs_proto::HeartbeatAck cHeartbeatAck;
	if (!cHeartbeatAck.ParseFromArray(m_BytesReadFromServer.data(), m_BytesReadFromServer.size()))
	{
		if (bVerbose) m_oLogger("[CFCMClient][ERROR] HandleHeartbeatAck: Cannot parse HeartbeatAck");
		return;
	}
	if (bVerbose) m_oLogger("[CFCMClient][INFO] Got kHeartbeatAckTag: "
		+ StringUtil::to_string(cHeartbeatAck.status()) + " "
		+ StringUtil::to_string(cHeartbeatAck.last_stream_id_received()) + " "
		+ StringUtil::to_string(cHeartbeatAck.stream_id()));

	//Send heartbeat every 600 seconds because server will close the connection if no heartbeat is received after 27 minutes
	std::thread([this, cHeartbeatAck]() {
		std::this_thread::sleep_for(std::chrono::milliseconds(TIME_SEND_HEARTBEAT));
		this->SendHeartbeat(cHeartbeatAck.last_stream_id_received());
		}).detach();
}

void CFCMClient::GetNextMessage()
{
	m_nMessageTag = 0;
	m_nMessageSize = 0;
	m_nState = MCS_TAG_AND_SIZE;
}