#include "LibCurlWrapper.h"

#include <curl/curl.h>
#include <string>
#include <vector>
#include <map>

CLibCurlWrapper::CLibCurlWrapper()
	: curl(nullptr)
	, curlHeaders(nullptr)
{
	curl_global_init(CURL_GLOBAL_ALL);
	curl = curl_easy_init();
}

CLibCurlWrapper::~CLibCurlWrapper()
{
    if (curl) {
        curl_easy_cleanup(curl);
    }
    curl_global_cleanup();
}

BOOL CLibCurlWrapper::Get(const std::string& sUrl, std::string& sResponse)
{
    std::string sDummy = "";
    return PerformRequest(sUrl, sDummy, sResponse, FALSE);
}

BOOL CLibCurlWrapper::Post(const std::string& sUrl, const std::string& sPostData, std::string& sResponse)
{
    return PerformRequest(sUrl, sPostData, sResponse, TRUE);
}

void CLibCurlWrapper::SetHeaders(const std::vector<std::string>& sHeaders)
{
    curl_slist_free_all(curlHeaders);
    curlHeaders = nullptr;

    for (const std::string& header : sHeaders)
    {
        curlHeaders = curl_slist_append(curlHeaders, header.c_str());
    }

    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, curlHeaders);
}

std::string CLibCurlWrapper::GetError()
{
    return m_sError;
}

size_t CLibCurlWrapper::WriteCallback(void* contents, size_t size, size_t nmemb, std::string* response)
{
    size_t total_size = size * nmemb;
    response->append(static_cast<char*>(contents), total_size);
    return total_size;
}

BOOL CLibCurlWrapper::PerformRequest(const std::string& sUrl, const std::string& sPostData, std::string& sResponse, BOOL bIsPost)
{
    if (!curl)
    {
        m_sError = "Error: CURL handle is not initialized.";
        return FALSE;
    }

    curl_easy_setopt(curl, CURLOPT_URL, sUrl.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &sResponse);

    if (bIsPost)
    {
        curl_easy_setopt(curl, CURLOPT_POST, 1L);
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, sPostData.c_str());
        curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, sPostData.length());
    }
    else
    {
        curl_easy_setopt(curl, CURLOPT_POST, 0L); // Ensure it's a GET request
    }

    CURLcode res = curl_easy_perform(curl);
    if (res != CURLE_OK)
    {
        m_sError = "Error: " + std::string(curl_easy_strerror(res));
        return FALSE;
    }

    return TRUE;
}
