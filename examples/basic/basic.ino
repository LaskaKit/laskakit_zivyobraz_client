#include <WiFi.h>
#include <zivyobrazclient.hpp>
#include <zdecoder.h>

// -- display dimensions --
static constexpr uint16_t WIDTH  = 800;
static constexpr uint16_t HEIGHT = 480;

// One row of raw Z color indices; reused by the decoder.
static uint8_t rowBuf[WIDTH];

// Decoder instance; created once WiFi is up.
static ZDec* decoder = nullptr;

// Called by the decoder after each complete row is written to rowBuf.
void onRow(const ZDecoder* dec)
{
    // dec->currentRow is the row that was just finished (zero-based).
    for (uint16_t x = 0; x < WIDTH; x++) {
        uint16_t rgb565 = ZtoRGB565(dec->rowBuffer[x], z2ColorToRGB565Lut, 4);
        // displayWritePixel(x, dec->currentRow, rgb565);
    }
}

// ContentHandler invoked for each received chunk of Z2 image data.
bool handleZ2(const uint8_t* data, size_t len)
{
    DecodeZ(&decoder->_decoder, data, len);
    return decoder->state() != ZERROR;
}

void setup()
{
    Serial.begin(115200);

    WiFi.begin("ssid", "password");
    while (WiFi.status() != WL_CONNECTED) delay(500);

    decoder = new ZDec(WIDTH, HEIGHT, rowBuf, onRow);

    LaskaKit::ZivyObraz::ZivyObrazClient client("https://cdn.zivyobraz.eu");
    client.setApiKey("your-api-key");
    client.registerHandler(LaskaKit::ZivyObraz::ContentType::IMAGE_Z2, handleZ2);

    // Build the status payload sent with every image request.
    // The server uses this to pick the right image for the display.
    String payload = R"({
    "apiVersion": "3.0",
    "board": "ESPink_V2",
    "fwVersion": "3.0",
    "system": {
        "cpuTemp": 44.3,
        "resetReason": "deepsleep",
        "vccVoltage": 4.10
    },
    "network": {
        "ssid": "ssid",
        "rssi": -60,
        "mac": "macaddr",
        "apRetries": 0,
        "lastDownloadDuration": 5,
        "ipAddress": "192.168.0.2"
    },
    "display": {
        "type": "GDEMxxx",
        "width": 800,
        "height": 480,
        "colorType": "4C",
        "lastRefreshDuration": 23
    },
    "sensors": [
        {
            "type": "SHT40",
            "temp": 23.4,
            "hum": 60.2
        }
    ]
})";

    int code = client.post("", payload);
    if (code == 200) {
        int bytes = client.readStream();
        Serial.printf("Received %d bytes\n", bytes);

        char sleep[16];
        if (client.getHeader(sleep, sizeof(sleep), "SleepSeconds")) {
            Serial.printf("Sleep for %s s\n", sleep);
        }
    } else {
        Serial.printf("HTTP error %d\n", code);
    }
}

void loop() {}
