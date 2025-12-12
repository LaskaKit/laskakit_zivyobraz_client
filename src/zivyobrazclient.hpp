#pragma once

#include <HTTPClient.h>

namespace LaskaKit::ZivyObraz {
    constexpr size_t MAX_URL_LENGTH = 256;
    constexpr size_t BUFFER_SIZE = 8192;  // 2^13

    // interesting headers to collect
    constexpr size_t COLLECT_HEADER_LEN = 7;
    static const char* COLLECT_HEADERS[COLLECT_HEADER_LEN] = {
        "Content-Type",
        "Content-Length",
        "Data-Length"
        "Timestamp",
        "Sleep",
        "SleepSeconds",
        "PreciseSleep",
        "Rotate",
    };

    enum class ContentType
    {
        APPLICATION_JSON,
        IMAGE_Z2,
        IMAGE_Z3,
        IMAGE_BMP,
        IMAGE_PNG,
        TEXT_PLAIN,
        TEXT_HTML,
        UNKNOWN,
    };

    enum class HTTPMethod
    {
        GET,
        POST
    };

    typedef void (*DownloadCallback)(ContentType contentType, const uint8_t* downloadedData, size_t datalen);

    class ZivyObrazClient {
    private:
        HTTPClient m_client;
        uint8_t m_requestBuffer[BUFFER_SIZE];
        size_t m_totalRead = 0;
        DownloadCallback downloadCallback;

        char m_baseUrl[50];
        char m_url[MAX_URL_LENGTH];
        char m_apiKey[20];

    public:
        ZivyObrazClient(const char* baseUrl, DownloadCallback downloadCallback);
        void setApiKey(const char* apiKey);
        int post(const char* path, const String& jsonPayload);
        int get(const char* path);
        bool getHeader(char* buf, const char* name);

    private:
        int sendRequest(const char* url, const char* method, const String& payload = "");
    };
}
