#pragma once

#include <HTTPClient.h>
#include <string.h>

namespace LaskaKit::ZivyObraz {

    // interesting headers from zivyobraz
    struct Headers
    {
        const char* headerKeys[5] = {
            "Content-Type",
            "Timestamp",
            "Sleep",
            "Rotate",
            "Data-Length"
        };

        const char* contentType = "unknown";
        int timestamp = -1;
        int sleep = -1;
        int rotate = -1;
        int dataLength = -1;

        void prepareCollect(HTTPClient& httpClient)
        {
            httpClient.collectHeaders(headerKeys, 5);
        }

        void collect(HTTPClient& httpClient)
        {
            if (httpClient.hasHeader("Content-Type")) {
                this->contentType = httpClient.header("Content-Type").c_str();
                Serial.println(this->contentType);
            }
            if (httpClient.hasHeader("Timestamp")) {
                this->timestamp = httpClient.header("Timestamp").toInt();
                Serial.println(this->timestamp);
            }
            if (httpClient.hasHeader("Sleep")) {
                this->sleep = httpClient.header("Sleep").toInt();
                Serial.println(this->sleep);
            }
            if (httpClient.hasHeader("Rotate")) {
                this->rotate =  httpClient.header("Rotate").toInt();
                Serial.println(this->rotate);
            }
            if (httpClient.hasHeader("Data-Length")) {
                this->dataLength = httpClient.header("Data-Length").toInt();
                Serial.println(this->dataLength);
            }
        }
    };


    class UrlBuilder
    {
    private:
        const char* baseUrl;

        size_t n_params = 0;
        char param_keys[10][20];
        char param_vals[10][20];

    public:
        UrlBuilder(const char* baseUrl)
            : baseUrl(baseUrl)
        {}

        bool addParam(const char* key, const char* val)
        {
            if (this->n_params >= 10) {
                return false;
            }
            strcpy(param_keys[this->n_params], key);
            strcpy(param_vals[this->n_params], val);
            this->n_params++;
            return true;
        }

        void build(char* url)
        {
            strcpy(url, this->baseUrl);
            for (int i = 0; i < this->n_params; i++) {
                if (i == 0) {
                    strcat(url, "?");
                } else {
                    strcat(url, "&");
                }
                strcat(url, this->param_keys[i]);
                strcat(url, "=");
                strcat(url, this->param_vals[i]);
            }
        }
    };


    class Client
    {
    private:
        HTTPClient httpClient;
        char url[256];

        size_t totalRead;

        public:
        Headers headers;
        Client(
            const String& host,
            const String& mac_address,
            const uint display_width,
            const uint display_height,
            const String& color_type,
            const String& firmware_version

        )
        {
            UrlBuilder builder(host.c_str());
            builder.addParam("mac", mac_address.c_str());
            builder.addParam("x", std::to_string(display_width).c_str());
            builder.addParam("y", std::to_string(display_height).c_str());
            builder.addParam("c", color_type.c_str());
            builder.addParam("fw", firmware_version.c_str());
            builder.build(this->url);


        }

        // read at most len bytes
        // returns number of bytes read
        int read(uint8_t* buf, size_t len)
        {
            int available = this->available();
            httpClient.getStream().read(buf + totalRead, available);
            totalRead += available;
        }

        size_t available()
        {
            return httpClient.getStream().available();
        }

        int download(uint8_t* buf)
        {
            Serial.println(this->url);
            this->httpClient.begin(url);

            // get headers
            this->headers.prepareCollect(httpClient);
            int httpCode = this->httpClient.GET();
            // Serial.println(httpCode);
            if (httpCode == -1) {
                return 0;
            }
            this->headers.collect(this->httpClient);
            // this->headers.print();
            // Serial.println(this->headers.contentType);

            int totalRead = 0;
            // 100 kb

            unsigned long startMillis = millis();
            while (httpClient.connected()) {
                size_t available = httpClient.getStream().available();
                // Serial.println(available);
                if (available) {
                    httpClient.getStream().read(buf + totalRead, available);
                    totalRead += available;
                }

                if (totalRead + available > 5000000) {
                    break;
                }
                delay(1);
            }

            if (httpClient.connected()) {
                Serial.println("HTTP connected");
            } else {
                Serial.println("HTTP disconnected");
            }

            Serial.print("Total read bytes: ");
            Serial.println(totalRead);

            Serial.print("Time taken [ms]: ");
            Serial.println(millis() - startMillis);

            Serial.print("Free heap: ");
            Serial.println(ESP.getFreeHeap());

            Serial.print("Max alloc heap: ");
            Serial.println(ESP.getMaxAllocHeap());

            Serial.print("RSSI: ");
            Serial.println(WiFi.RSSI());
            return totalRead;
        }
    };
}
