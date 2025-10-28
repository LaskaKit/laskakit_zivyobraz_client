// #pragma once

// #include <cstdlib>
// #include <cstdint>
// #include <memory>

// #include "zivyobrazclient.hpp"

// namespace LaskaKit::ZivyObraz {
//     constexpr size_t BMP_MAX_ROW_SIZE = 800;

//     struct BMPHeader  {
//         uint16_t type;
//         uint32_t size;
//         uint32_t reserved;
//         uint32_t offset;
//     } __attribute__((__packed__));

//     struct BMPInfoHeader {
//         uint32_t headerSize;
//         uint32_t width;
//         uint32_t height;
//         uint16_t planes;
//         uint16_t bitsPerPixel;
//         uint32_t compression;
//         uint32_t imageSize;
//         uint32_t xPixelsPerM;
//         uint32_t yPixelsPerM;
//         uint32_t colorsUsed;
//         uint32_t importantColors;
//     } __attribute__((__packed__));

//     struct BMPColorTable
//     {
//         Pixel colors[16];
//     };

//     struct BMPPixel {
//         uint8_t r;
//         uint8_t g;
//         uint8_t b;
//     } __attribute__((__packed__));

//     // suports continuouts decoding
//     class BMPDEC
//     {
//     private:
//         BMPHeader header;
//         BMPInfoHeader infoHeader;
//         BMPColorTable colorTable;
//         bool headerRead;
//         bool infoHeaderRead;
//         Pixel rowData[BMP_MAX_ROW_SIZE];
//         ZIVYOBRAZ_DRAW_CALLBACK callback;
//     public:
//         // copies data to input buffer and continues parsing
//         void newData(const uint8_t* data, size_t datalen);
//         void newPixelData(const uint8_t* data, size_t datalen);
//         void addCallback(ZIVYOBRAZ_DRAW_CALLBACK callback)
//         {
//             this->callback = callback;
//         }
//     };

//     class StreamingBMPDEC
//     {
//     private:
//         BMPHeader header;
//         BMPInfoHeader infoHeader;
//         BMPColorTable colorTable;
//         Pixel rowData[BMP_MAX_ROW_SIZE];

//         uint16_t currentRow = 0;
//         uint16_t currentCol = 0;
//         bool headersLoaded = false;
//         Pixel bufferedPixel;
//         uint8_t bufferedPixelIdx;

//         ZIVYOBRAZ_DRAW_CALLBACK callback;

//     public:
//         StreamingBMPDEC() = default;

//         void addCallback(ZIVYOBRAZ_DRAW_CALLBACK callback)
//         {
//             this->callback = callback;
//         }
//         void reset();
//         void newData(const uint8_t* data, size_t datalen);

//     private:
//         void newHeaderData(const uint8_t* data, size_t datalen);
//         void newPixelData(const uint8_t* data, size_t datalen);

//     };
// }
