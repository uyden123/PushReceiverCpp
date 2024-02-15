#pragma once

#include <curl/curl.h>
#include <string>
#include <vector>
#include <map>

class CLibCurlWrapper
{
public:
    CLibCurlWrapper();
    ~CLibCurlWrapper();


    /**
     * Performs a GET request to the specified URL.
     * 
     * @param sUrl The URL to send the GET request to.
     * @param sResponse [out] The response received from the server.
     * @return TRUE if the request was successful, FALSE otherwise.
     */
    BOOL Get(const std::string& sUrl, std::string& sResponse);

    /**
     * Performs a POST request to the specified URL.
     * 
     * @param sUrl The URL to send the POST request to.
     * @param sPostData The data to be sent in the POST request.
     * @param sResponse [out] The response received from the server.
     * @return TRUE if the request was successful, FALSE otherwise.
     */
    BOOL Post(const std::string& sUrl, const std::string& sPostData, std::string& sResponse);

    /**
     * Sets the headers to be included in the HTTP request.
     * 
     * @param sHeaders The vector of headers to be set.
     */
    void SetHeaders(const std::vector<std::string>& sHeaders);

    /**
     * Gets the error message associated with the last request.
     * 
     * @return The error message.
     */
    std::string GetError();

private:

    /**
     * Callback function used by libcurl to write response data.
     * 
     * @param contents Pointer to the response data.
     * @param size The size of each element in the response data.
     * @param nmemb The number of elements in the response data.
     * @param response Pointer to the string where the response data will be stored.
     * @return The total number of bytes written.
     */
    static size_t WriteCallback(void* contents, size_t size, size_t nmemb, std::string* response);

    /**
     * Performs an HTTP request to the specified URL.
     * 
     * @param sUrl The URL to send the request to.
     * @param sPostData The data to be sent in the request (optional).
     * @param sResponse [out] The response received from the server.
     * @param bIsPost Specifies whether the request is a POST request (default is GET).
     * @return TRUE if the request was successful, FALSE otherwise.
     */
    BOOL PerformRequest(const std::string& sUrl, const std::string& sPostData, std::string& sResponse, BOOL bIsPost);

private:
    CURL* curl;
    curl_slist* curlHeaders = nullptr;

    std::string m_sError;
};
