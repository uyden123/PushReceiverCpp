#pragma once

#include "mcs.pb.h"
#include "Emitter.h"
#include "FCMRegister.h"
#include "Http_ece/ece.h"
#include "SecureSocket/TCPSSLClient.h"

#define RS_LENGTH 4096
#define TIME_SEND_HEARTBEAT 600000 // 10 minutes

enum ProcessingState
{
	MCS_VERSION_TAG_AND_SIZE,
	MCS_TAG_AND_SIZE,
	MCS_SIZE,
	MCS_PROTO_BYTES
};

enum MCSProtoTag
{
	kHeartbeatPingTag,
	kHeartbeatAckTag,
	kLoginRequestTag,
	kLoginResponseTag,
	kCloseTag,
	kMessageStanzaTag,
	kPresenceStanzaTag,
	kIqStanzaTag,
	kDataMessageStanzaTag,
	kBatchPresenceStanzaTag,
	kStreamErrorStanzaTag,
	kHttpRequestTag,
	kHttpResponseTag,
	kBindAccountRequestTag,
	kBindAccountResponseTag,
	kTalkMetadataTag,
	kNumProtoTypes
};

constexpr int kVersionPacketLen = 1;
constexpr int kTagPacketLen = 1;
constexpr int kSizePacketLenMin = 1;
constexpr int kSizePacketLenMax = 5;
constexpr int kMCSVersion = 41;

constexpr bool bVerbose = true;

class CFCMClient : public CEmitter
{
public:

    /**
     * @brief Constructs a new instance of the CFCMClient class.
     * 
     * @param oLogger The callback function for logging.
     * @param sAndroidID The Android ID.
     * @param sSecurityToken The security token.
     * @param sBase64PrivateKey The base64 encoded private key.
     * @param sBase64AuthSecret The base64 encoded authentication secret.
     */
    CFCMClient(const LogFnCallback oLogger,
               const std::string sAndroidID,
               std::string sSecurityToken,
               std::string sBase64PrivateKey,
               std::string sBase64AuthSecret,
			   std::vector<std::string> persistentIDs);

	~CFCMClient();

	bool ConnectToServer();
	void StartReceiver();

private:
	void SendLoginBuffer();
	void SendHeartbeat(int32_t nLastStreamIDReceived);
	int CalculateMinBytesNeeded();
	void ProcessData();
	void GotVersion();
	void GotMessageTag();
	void GotMessageSize();
	void GotMessageBytes();
	void HandleLoginResponseTag();
	void HandleIqStanzaTag();
	void HandleDataMessageStanzaTag();
	void HandleHeartbeatAck();
	void GetNextMessage();

private:
	std::unique_ptr<CTCPSSLClient> m_SecureTCPClient;
	const LogFnCallback m_oLogger;

	const char* m_szHost = "mtalk.google.com";
	const char* m_szPort = "5228";

	int m_nState;
	int m_nSizePacketSoFar;
	int m_nMessageTag;
	int m_nMinBytesNeeded;
	uint32_t m_nMessageSize;

	std::vector<uint8_t> m_BytesReadFromServer;

	std::string m_sAndroidId;
	std::string m_sSecurityToken;
	std::vector<uint8_t> m_RawSubPrivKey;
	std::vector<uint8_t> m_AuthSecret;

	std::vector<std::string> m_PersistentIds;
};