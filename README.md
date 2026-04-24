# LaskaKit ZivyObraz Client

Arduino library for [zivyobraz.eu](https://zivyobraz.eu) — a server that serves images to low-power ESP32 e-ink displays.

The library has two independent parts:

| Component | Header | Purpose |
|-----------|--------|---------|
| `ZivyObrazClient` | `zivyobrazclient.hpp` | HTTP client — fetches images and dispatches response chunks to a registered handler |
| `ZDec` / `ZDecoder` | `zdecoder.h` | RLE decoder for the Z2 and Z3 image formats |

## Z image formats

**Z2** — 2-bit RLE: 4 colors, 6-bit run length per byte.  
**Z3** — 3-bit RLE: 8 colors, 5-bit run length per byte.  
Both streams start with a 2-byte ASCII header (`"Z2"` or `"Z3"`) followed by raw RLE bytes.

The decoder outputs raw color indices row by row via a callback. Convert indices to RGB565 with `ZtoRGB565()` and the built-in lookup tables (`z2ColorToRGB565Lut`, `z3ColorToRGB565Lut`, or their grayscale variants), or supply your own.

## Response headers

The server sends these headers; retrieve them with `getHeader()` after `get()`/`post()`:

| Header | Description |
|--------|-------------|
| `Timestamp` | Image creation time |
| `Sleep` / `SleepSeconds` / `PreciseSleep` | How long the device should deep-sleep before the next refresh |
| `Rotate` | Suggested display rotation |
| `Data-Length` | Byte length of the image payload |

## Example

```cpp
#include <WiFi.h>
#include "zivyobrazclient.hpp"
#include "zdecoder.h"

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
```

## Installation

**Arduino IDE (Library Manager)** — search for **LaskakitZivyobrazClient** and click Install. (soon...)

**Arduino IDE (manual)** — download or clone this repository, then in the IDE go to
*Sketch → Include Library → Add .ZIP Library…* and select the downloaded archive,
or copy the repository folder directly into your Arduino `libraries/` directory and restart the IDE.

**PlatformIO** — add the GitHub URL to `lib_deps` in `platformio.ini`:

```ini
[env:esp32]
platform = espressif32
board = esp32dev
framework = arduino
lib_deps =
    https://github.com/LaskaKit/laskakit_zivyobraz_client
```

Requires the ESP32 Arduino core (`HTTPClient.h`).
