
#include <HTTPClient.h>

#include "zivyobrazclient.hpp"

namespace LaskaKit::ZivyObraz {
    constexpr size_t BUFFER_SIZE = 8192;  // 2^13

    enum class ContentType
    {
        APPLICATION_JSON,
        IMAGE_Z2,
        IMAGE_Z3,
        IMAGE_BMP,
        IMAGE_PNG,
        TEXT_PLAIN,
        UNKNOWN,
    };

    class ESPClient;  // forward declaration
    typedef void (*DownloadCallback)(ContentType contentType, const uint8_t* downloadedData, size_t datalen);

    class EspClient : public ZivyobrazClient {
    private:
        HTTPClient m_client;
        uint8_t m_requestBuffer[BUFFER_SIZE];
        size_t m_totalRead = 0;
        DownloadCallback downloadCallback;

    public:
        EspClient(const char* host, DownloadCallback downloadCallback)
            : downloadCallback(downloadCallback)
            // : ZivyobrazClient()
        {
            strncpy(this->m_url, host, MAX_URL_LENGTH);
        }

        void get()
        {
            constexpr const char* path = "/?";
            strncat(this->m_url, path, MAX_URL_LENGTH - strlen(this->m_url));
            this->m_httpParams.buildQuery(this->m_url, MAX_URL_LENGTH);
            Serial.printf("Connecting to: %s\n", this->m_url);
            m_client.begin(this->m_url);
            m_client.addHeader("Content-Type", "application/json");
            const char* headerKeys[6] = {
                "Content-Type",
                "Timestamp",
                "Sleep",
                "SleepSeconds",
                "Rotate",
                "Data-Length"
            };

            m_client.collectHeaders(headerKeys, 6);

            int httpCode = m_client.GET();
            ContentType contentType = ContentType::UNKNOWN;
            if (m_client.hasHeader("Content-Type")) {
                String ct = m_client.header("Content-Type");
                if (ct == "application/json") {
                    contentType = ContentType::APPLICATION_JSON;
                } else if (ct == "image/z2") {
                    contentType = ContentType::IMAGE_Z2;
                } else if (ct == "image/z3") {
                    contentType = ContentType::IMAGE_Z3;
                } else if (ct == "image/bmp") {
                    contentType = ContentType::IMAGE_BMP;
                } else if (ct == "image/png") {
                    contentType = ContentType::IMAGE_PNG;
                } else if (ct == "text/plain") {
                    contentType = ContentType::TEXT_PLAIN;
                }
            }

            this->m_totalRead = 0;
            while (m_client.connected()) {
                size_t available = m_client.getStream().available();

                if (available) {
                    size_t m_processed = 0;

                    while (m_processed < available) {
                        size_t to_process = available - m_processed < BUFFER_SIZE ? available - m_processed : BUFFER_SIZE;
                        printf("Downloading: %lu bytes\n", to_process);
                        m_client.getStream().read(m_requestBuffer, to_process);

                        this->downloadCallback(contentType, m_requestBuffer, to_process);
                        m_totalRead += to_process;
                        m_processed += to_process;
                    }
                }
                delay(1);
            }
        }

        void getHeader(char* buf, const char* name)
        {
            if (m_client.hasHeader(name)) {
                strcpy(buf, m_client.header(name).c_str());
            } else {
                strcpy(buf, "");
            }
        }

    };
}
