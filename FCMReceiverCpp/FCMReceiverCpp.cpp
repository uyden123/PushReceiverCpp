#define _SILENCE_EXPERIMENTAL_FILESYSTEM_DEPRECATION_WARNING
#include <iostream>
#include <string>
#include <fstream>
#include <chrono>
#include <ctime>
#include <locale>
#include <codecvt>

#include <experimental/filesystem>

#include "FCMClient.h"
#include "FCMRegister.h"
#include "ArgumentParser.h"

#include "json.hpp"
using json = nlohmann::json;

std::wstring g_sLogPath = L"FCMReceiver.log";

//define exit codes
enum ExitCode 
{
	SUCCESS = 0,
	PARSE_ERROR,
	ARGUMENT_ERROR,
	CANT_READ_REGISTER_INPUT_FILE,
	REGISTER_INPUT_FILE_TYPE_INVALID,
	REGISTER_INPUT_DATA_INVALID,
	CANT_GENERATE_KEYS,
	REGISTER_OUTPUT_FILE_TYPE_INVALID,
	REGISTER_FAILED,
	CANT_WRITE_REGISTER_DATA,
	LISTEN_INPUT_FILE_TYPE_INVALID,
	CANT_READ_LISTEN_INPUT_FILE,
	LISTEN_INPUT_DATA_INVALID,
	CANT_CONNECT_FCM_SERVER,
	ERROR_WHILE_LISTENING
};

bool IsFolderExist(const std::wstring& sFolder)
{
	return std::experimental::filesystem::exists(sFolder) && std::experimental::filesystem::is_directory(sFolder);
}

bool WriteRegisterDataToFile(const FCM_REGISTER_DATA_RETURN& data, const std::wstring& filename)
{
	json j = json{ {"acg", {{"ID", data.acg.sID}, {"SecurityToken", data.acg.sSecurityToken}}},
					{"ece", {{"AuthSecret", data.ece.sAuthSecret}, {"PrivateKey", data.ece.sPrivateKey}}},
					   {"Token", data.sToken} };

	std::ofstream file(filename);
	if (!file.is_open())
		return false;

	file << j.dump(4);
	file.close();

	return true;
}

bool LoadJsonFromFile(const std::wstring& sFilename, json& jsonData)
{
	std::ifstream file(sFilename);
	if (!file.is_open())
		return false;

	try {
		file >> jsonData;
	}
	catch (json::parse_error& e)
	{
		return false;
	}

	file.close();
	return true;
}

bool IsInitFCMDataValid(const json& data)
{
	if (!data.contains("appid") || !data.contains("projectid") || !data.contains("apikey") || !data.contains("vapidkey"))
		return false;

	return true;
}

bool IsRegisterDataValid(const json& data)
{
	if (!data.contains("acg") || !data.contains("ece") || !data.contains("Token"))
		return false;

	if (!data["acg"].contains("ID") || !data["acg"].contains("SecurityToken") ||
				!data["ece"].contains("AuthSecret") || !data["ece"].contains("PrivateKey") ||
				!data.contains("Token"))
		return false;

	return true;
}

bool WritePersistentIDToFile(const std::wstring& sFilename, const std::string& sPersistentIDs)
{
	std::ofstream file(sFilename);
	if (!file.is_open())
	{
		return false;
	}

	file << sPersistentIDs;
	file.close();
	return true;
}

bool LoadPersistentID(const std::wstring& sFilename, std::vector<std::string>& persistentIDs)
{
	std::ifstream file(sFilename);
	if (!file.is_open())
		return false;

	std::string sPersistentID;
	file >> sPersistentID;
	file.close();

	persistentIDs = StringUtil::split(sPersistentID, ';');

	return true;
}

auto MyLogPrinter = [](const std::string& strLogMsg)
	{
		auto now = std::chrono::system_clock::now();
		std::time_t currentTime = std::chrono::system_clock::to_time_t(now);

		std::stringstream ss;
		ss << std::put_time(std::localtime(&currentTime), "%H:%M:%S %d-%m-%Y");
		std::string timeStr = ss.str();

		std::cout << "[" << timeStr << "] " << strLogMsg << std::endl;

		std::ofstream file(g_sLogPath, std::ios::app);
		if (file.is_open())
		{
			file << "[" << timeStr << "] " << strLogMsg << std::endl;
			file.close();
		}
		else
		{
			std::ofstream newFile(g_sLogPath);
			if (newFile.is_open())
			{
				newFile << "[" << timeStr << "] " << strLogMsg << std::endl;
				newFile.close();
			}
			else
			{
				std::cerr << "Failed to create the log file." << std::endl;
			}
		}
	};

int wmain(int argc, wchar_t* argv[])
{
	CArgumentParser cArgumentParser(argc, argv, L"FCMReceiverCpp", L"v1.0.0", L"FCM Push notification receiver by UyBH");

	CArgumentOption cRegisterOption({ 'r' }, { L"register" }, L"Register to fcm server");
	CArgumentOption cRegisterInputFileOption(ArgumentOptionType::InputOption, { }, {L"register_input" }, L"If set, the register info will be taken from this path. Otherwise, the system will attempt to find 'init_fcm_data.json' in the same directory as this executable being called.");
	CArgumentOption cRegisterOutputFileOption(ArgumentOptionType::InputOption, { }, {L"register_output" }, L"If set, the register info file will be placed in this path. Otherwise, it will be placed in the same directory as this executable being called.");

	CArgumentOption cListenOption({ 'l' }, { L"listen" }, L"Listen to fcm server");
	CArgumentOption cListenInputFileOption(ArgumentOptionType::InputOption, { }, {L"listen_input" }, L"If set, the register info will be taken from this path. Otherwise, the system will attempt to find 'fcm_register_data.json' in the same directory as this executable being called.");

	CArgumentOption cLogPathOption(ArgumentOptionType::InputOption, { }, { L"log_folder" }, L"If set, log file 'FCMReceiver.log' will be placed in this folder. Otherwise, it will be placed in the same folder as this executable being called.");

	CArgumentOption helpOption(ArgumentOptionType::HelpOption, { 'h' }, { L"help" }, L"Prints out this message.");
	CArgumentOption versionOption(ArgumentOptionType::VersionOption, { 'v' }, { L"version" }, L"Prints out the version.");
	cArgumentParser.AddArgumentOption({
		&cListenInputFileOption,
		&cListenOption,
		&cRegisterInputFileOption,
		&cRegisterOutputFileOption,
		&cRegisterOption,
		&cLogPathOption,
		&helpOption,
		&versionOption
		});

	ParseResult parseResult = cArgumentParser.Parse();
	if (parseResult != ParseResult::ParseSuccessful) 
	{
		std::wcout << cArgumentParser.ErrorText();
		exit(ExitCode::PARSE_ERROR);
	}

	if (cRegisterOption.WasSet() > 1 || 
		cRegisterInputFileOption.WasSet() > 1 ||
		cRegisterOutputFileOption.WasSet() > 1 ||
		cLogPathOption.WasSet() > 1 ||
		cListenOption.WasSet() > 1 ||
		cListenInputFileOption.WasSet() > 1) 
	{
		std::wcout << "Error: Option was set more than once.";
		exit(ExitCode::ARGUMENT_ERROR);
	}

	if (helpOption.WasSet() && versionOption.WasSet()) {
		std::wcout << cArgumentParser.VersionText() << cArgumentParser.HelpText();
		exit(ExitCode::SUCCESS);
	}

	if (helpOption.WasSet()) {
		std::wcout << cArgumentParser.HelpText();
		exit(ExitCode::SUCCESS);
	}

	if (versionOption.WasSet()) {
		std::wcout << cArgumentParser.VersionText();
		exit(ExitCode::SUCCESS);
	}

	if (cLogPathOption.WasSet())
	{
		std::wstring sFolderPath = cLogPathOption.GetValue();
		
		if (!IsFolderExist(sFolderPath))
		{
			std::wcout << "Log folder does not exist. use default folder path." << std::endl;
		}
		else
		{
			g_sLogPath = sFolderPath + L"/FCMReceiver.log";
		}
	}

	if (cRegisterOption.WasSet())
	{
		std::wstring sRegisterInputFilePath = cRegisterInputFileOption.WasSet()
			? cRegisterInputFileOption.GetValue() : L"init_fcm_data.json";

		if (sRegisterInputFilePath.size() < 5 || sRegisterInputFilePath.substr(sRegisterInputFilePath.size() - 5) != L".json")
		{
			std::cerr << "Register input file must be a json file." << std::endl;
			exit(ExitCode::REGISTER_INPUT_FILE_TYPE_INVALID);
		}

		json fcmInitData;
		if (!LoadJsonFromFile(sRegisterInputFilePath, fcmInitData))
		{
			std::wcerr << "Unable to read from file: " << sRegisterInputFilePath << std::endl;
			exit(ExitCode::CANT_READ_REGISTER_INPUT_FILE);
		}

		if (!IsInitFCMDataValid(fcmInitData))
		{
			std::cerr << "Register input init data is invalid." << std::endl;
			exit(ExitCode::REGISTER_INPUT_DATA_INVALID);
		}

		std::wstring sOutputFileName = cRegisterOutputFileOption.WasSet()
			? cRegisterOutputFileOption.GetValue() : L"fcm_register_data.json";

		if (sOutputFileName.size() < 5 || sOutputFileName.substr(sOutputFileName.size() - 5) != L".json")
		{
			std::cerr << "Register output file must be a json file." << std::endl;
			exit(ExitCode::REGISTER_OUTPUT_FILE_TYPE_INVALID);
		}

		ECDH_KEYS keys;
		int nError = UtilFunction::GenerateECDHKeys(keys);
		if (nError != ECE_OK)
		{
			std::cerr << "Unable to generate ECDH keys." << std::endl;
			exit(ExitCode::CANT_GENERATE_KEYS);
		}

		FCM_PARAMS params;
		params.firebase.sAppID = fcmInitData["appid"];
		params.firebase.sProjectID = fcmInitData["projectid"];
		params.firebase.sApiKey = fcmInitData["apikey"];
		params.sVapidKey = fcmInitData["vapidkey"];
		params.keys = keys;

		CFCMRegister cFCMRegister(MyLogPrinter);
		FCM_REGISTER_DATA_RETURN fcmRegisterData = cFCMRegister.RegisterToFCM(params);

		if (fcmRegisterData.sToken.empty() || fcmRegisterData.acg.sID.empty() || fcmRegisterData.acg.sSecurityToken.empty())
			exit(ExitCode::REGISTER_FAILED);

		if (!WriteRegisterDataToFile(fcmRegisterData, sOutputFileName))
		{
			std::wcerr << "Unable to write to file: " << sOutputFileName << std::endl;
			exit(ExitCode::CANT_WRITE_REGISTER_DATA);
		}

		std::wcout << "FCM registration data has been saved to " << sOutputFileName << std::endl;

		exit(ExitCode::SUCCESS);
	}

	if (cListenOption.WasSet())
	{
		std::wstring sListenInputFilePath = cListenInputFileOption.WasSet()
			? cListenInputFileOption.GetValue() : L"fcm_register_data.json";

		if (sListenInputFilePath.size() < 5 || sListenInputFilePath.substr(sListenInputFilePath.size() - 5) != L".json")
		{
			std::cerr << "Listen input file must be a json file." << std::endl;
			exit(ExitCode::LISTEN_INPUT_FILE_TYPE_INVALID);
		}

		json fcmRegisterData;
		if (!LoadJsonFromFile(sListenInputFilePath, fcmRegisterData))
		{
			std::wcerr << "Unable to read from file: " << sListenInputFilePath << std::endl;
			exit(ExitCode::CANT_READ_LISTEN_INPUT_FILE);
		}

		if (!IsRegisterDataValid(fcmRegisterData))
		{
			std::cerr << "Listen input register data is invalid." << std::endl;
			exit(ExitCode::LISTEN_INPUT_DATA_INVALID);
		}

		//This persistent id is used to tell to the server that what messages we have already received
		//and server will send only new messages so we must save this id to a file and read it from there
		//when we start new connection to the server
		std::vector<std::string> persistentIDs;
		LoadPersistentID(L"persistent_id.txt", persistentIDs);

		MyLogPrinter("Receiving message in FCM token: " + fcmRegisterData["Token"].dump());

		CFCMClient cFCMClient(MyLogPrinter,
			fcmRegisterData["acg"]["ID"],
			fcmRegisterData["acg"]["SecurityToken"],
			fcmRegisterData["ece"]["PrivateKey"],
			fcmRegisterData["ece"]["AuthSecret"],
			persistentIDs);

		cFCMClient.Once("connected", MyLogPrinter);

		cFCMClient.On("persistent_id", [](const std::string& sPersistentID) {
			//MyLogPrinter("[MAIN][INFO] persistent_id list: " + sPersistentID);

			if (!WritePersistentIDToFile(L"persistent_id.txt", sPersistentID))
			{
				MyLogPrinter("[MAIN][WARNING] Unable to write persistent id to file.");
			}
		});

		cFCMClient.On("message", [](const std::string& message) {
			MyLogPrinter("[MAIN][INFO] Message: " + message);
		});

		if (cFCMClient.ConnectToServer())
		{
			try {
				cFCMClient.StartReceiver();
			}
			catch (std::exception& e)
			{
				MyLogPrinter("[MAIN][ERROR] " + std::string(e.what()));
				exit(ExitCode::ERROR_WHILE_LISTENING);
			}
		}
		else
		{
			exit(ExitCode::CANT_CONNECT_FCM_SERVER);
		}

		exit(ExitCode::SUCCESS);
	}

	std::wcout << cArgumentParser.HelpText();
	exit(ExitCode::SUCCESS);

	return 0;
}