
#include <HTTPClient.h>

#include "zivyobrazclient.hpp"
#include "BMPDEC.hpp"
#include "ZDEC.hpp"

namespace LaskaKit::ZivyObraz {
    constexpr size_t MAX_URL_LENGTH = 256;


    class EspClient {
    private:
        HttpParams params;
        HTTPClient client;
        char url[MAX_URL_LENGTH];
        uint8_t* buf;
        size_t totalRead;

    public:
        EspClient(const char* host)
        {
            strncpy(this->url, host, MAX_URL_LENGTH);
        }

        ~EspClient()
        {
            free(this->buf);
        }

        void addParam(const char* key, const char* value)
        {
            this->params.add(key, value);
        }

        void get()
        {
            constexpr const char* path = "/?";
            strncat(this->url, path, MAX_URL_LENGTH - strlen(this->url));
            this->params.buildQuery(url, MAX_URL_LENGTH);
            Serial.printf("Connecting to: %s\n", this->url);
            this->client.begin(this->url);
            
            
            const char* headerKeys[5] = {
                "Content-Type",
                "Timestamp",
                "Sleep",
                "Rotate",
                "Data-Length"
            };
            
            this->client.collectHeaders(headerKeys, 5);
            
            this->client.GET();

            // dont forget to download headers
            Serial.println("Try to get header");
            if (this->client.hasHeader("Timestamp")) {
                Serial.println("Has header");
            }
            if (this->client.hasHeader("Data-Length")) {
                Serial.println("Has header");
            }

            this->buf = (uint8_t*)malloc(5000000 * sizeof(uint8_t));
            // TODO very careful here
            if (this->buf == NULL) {
                Serial.println("Memory allocation failed.");
                return;
            }

            this->totalRead = 0;
            while (this->client.connected()) {
                size_t available = this->client.getStream().available();
                // Serial.println(available);
                if (available) {
                    this->client.getStream().read(this->buf + this->totalRead, available);
                    this->totalRead += available;
                }

                if (this->totalRead + available > 5000000) {
                    break;
                }
                delay(1);
            }
        }

        void getHeader(char* buf, const char* name)
        {
            if (this->client.hasHeader(name)) {
                strcpy(buf, this->client.header(name).c_str());
            }
        }

        void processBMP(ZIVYOBRAZ_DRAW_CALLBACK callback) {
            BMPDEC decoder;
            decoder.addCallback(callback);
            decoder.newData(reinterpret_cast<const uint8_t*>(this->buf), this->totalRead);
        }

        void processZ(ZIVYOBRAZ_DRAW_CALLBACK callback, uint8_t type) {
            ZDEC decoder(800, 480, type);
            decoder.addCallback(callback);
            decoder.newData(reinterpret_cast<const uint8_t*>(this->buf), this->totalRead);
        }

        void process(ZIVYOBRAZ_DRAW_CALLBACK callback)
        {
            printf("%c%c\n", this->buf[0], this->buf[1]);
            if (this->buf[0] == 'B' && this->buf[1] == 'M') {
            this->processBMP(callback);
            } else if (this->buf[0] == 'Z') {
                switch (this->buf[1]) {
                    case '1':
                        this->processZ(callback, 1);
                        break;
                    case '2':
                        this->processZ(callback, 2);
                        break;
                    case '3':
                        this->processZ(callback, 3);
                        break;
                }
            }
        }
    };
}
