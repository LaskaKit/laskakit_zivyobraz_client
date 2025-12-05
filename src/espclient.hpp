
#include <HTTPClient.h>

#include "zivyobrazclient.hpp"

namespace LaskaKit::ZivyObraz {
    constexpr size_t BUFFER_SIZE = 8192;  // 2^13

    template<class T_DECODER>
    class EspClient : public ZivyobrazClient<T_DECODER>{
    private:
        HTTPClient m_client;
        uint8_t m_requestBuffer[BUFFER_SIZE];
        size_t m_totalRead = 0;

    public:
        EspClient(const char* host, T_DECODER* decoder)
            : ZivyobrazClient<T_DECODER>(decoder)
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

            this->m_totalRead = 0;
            while (m_client.connected()) {
                size_t available = m_client.getStream().available();

                if (available) {
                    size_t m_processed = 0;

                    while (m_processed < available) {
                        size_t to_process = available - m_processed < BUFFER_SIZE ? available - m_processed : BUFFER_SIZE;
                        printf("READING: %lu bytes\n", to_process);
                        m_client.getStream().read(m_requestBuffer, to_process);

                        this->m_decoder->decode(m_requestBuffer, to_process);
                        m_totalRead += to_process;
                        m_processed += to_process;
                    }
                }

                if (m_totalRead + available > 5000000) {
                    break;
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
