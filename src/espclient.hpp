
#include <HTTPClient.h>

#include "zivyobrazclient.hpp"
#include "BMPDEC.hpp"
#include "ZDEC.hpp"

namespace LaskaKit::ZivyObraz {
    constexpr size_t MAX_URL_LENGTH = 256;
    constexpr size_t BUFFER_SIZE = 2048;


    class EspClient {
    private:
        HttpParams params;
        HTTPClient client;
        char url[MAX_URL_LENGTH];
        uint8_t buf[BUFFER_SIZE];
        size_t totalRead;

        StreamingZDEC zDecoder;
        StreamingBMPDEC bmpDecoder;

        ZIVYOBRAZ_DRAW_CALLBACK callback;


    public:
        EspClient(const char* host, size_t displayWidth, size_t displayHeight, ZIVYOBRAZ_DRAW_CALLBACK callback)
        {
            strncpy(this->url, host, MAX_URL_LENGTH);
            this->zDecoder = StreamingZDEC(800, 480, 2);
            this->zDecoder.addCallback(callback);
            this->bmpDecoder.addCallback(callback);
        }

        virtual ~EspClient() {}

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
            
            const char* headerKeys[6] = {
                "Content-Type",
                "Timestamp",
                "Sleep",
                "SleepSeconds",
                "Rotate",
                "Data-Length"
            };
            
            this->client.collectHeaders(headerKeys, 6);
            int httpCode = this->client.GET();

            this->totalRead = 0;
            int image_type = 0;
            while (this->client.connected()) {
                size_t available = this->client.getStream().available();

                if (available) {
                    size_t processed = 0;

                    while (processed < available) {
                        size_t to_process = available - processed < BUFFER_SIZE ? available - processed : BUFFER_SIZE;
                        printf("READING: %lu bytes\n", to_process);
                        this->client.getStream().read(this->buf, to_process);

                        if (this->totalRead == 0) {
                            char one = this->buf[0];
                            char two = this->buf[1];
                            if (one == 'B' && two == 'M') {
                                image_type = 1;
                            } else if (one == 'Z') {
                                image_type = 2;
                            }
                            printf("%c%c\n", this->buf[0], this->buf[1]);
                            // TODO -> can deduce type from here
                        }

                        switch (image_type) {
                            case 1:
                                this->processBMP(to_process);
                                break;
                            case 2:
                                this->processZ(2, to_process);
                                printf("Processing Z\n");
                                break;
                        }
                        this->totalRead += to_process;
                        processed += to_process;
                    }
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

        void processBMP(size_t available) {
            this->bmpDecoder.newData(reinterpret_cast<const uint8_t*>(this->buf), available);
        }

        void processZ(uint8_t type, size_t available) {
            this->zDecoder.newData(reinterpret_cast<const uint8_t*>(this->buf), available);
        }

        void process(size_t available)
        {
            printf("%c%c\n", this->buf[0], this->buf[1]);
            if (this->buf[0] == 'B' && this->buf[1] == 'M') {
            this->processBMP(available);
            } else if (this->buf[0] == 'Z') {
                switch (this->buf[1]) {
                    case '1':
                        this->processZ(1, available);
                        break;
                    case '2':
                        this->processZ(2, available);
                        break;
                    case '3':
                        this->processZ(3, available);
                        break;
                }
            }
        }
    };
}
