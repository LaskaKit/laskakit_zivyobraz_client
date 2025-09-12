#include <cstring>
#include <cstdlib>
#include <cstring>
#include <cstdio>

#include "BMPDEC.hpp"

using namespace LaskaKit::ZivyObraz;

void BMPDEC::newData(const uint8_t* data, size_t datalen)
{
    if (!this->headerRead) {
        memcpy(&this->header, data, sizeof(this->header));
        this->headerRead = true;
    }

    if (!this->infoHeaderRead) {
        memcpy(&this->infoHeader, data + sizeof(this->header), sizeof(this->infoHeader));
        this->infoHeaderRead = true;
    }

    printf("File type: %c%c\n", header.type, header.type >> 8);
    printf("File size: %u\n", header.size);
    printf("Offset: %u\n", header.offset);

    printf("Header size: %u\n", infoHeader.headerSize);
    printf("Width: %u\n", infoHeader.width);
    printf("Height: %u\n", infoHeader.height);
    printf("Planes: %u\n", infoHeader.planes);
    printf("Bits per pixel: %u\n", infoHeader.bitsPerPixel);
    printf("Compression: %u\n", infoHeader.compression);
    printf("Image size: %u\n", infoHeader.imageSize);
    printf("X pixels per m: %u\n", infoHeader.xPixelsPerM);
    printf("Y pixels per m: %u\n", infoHeader.yPixelsPerM);
    printf("Colors used: %u\n", infoHeader.colorsUsed);
    printf("Important colors: %u\n", infoHeader.importantColors);
    printf("Read so far: %ld\n", sizeof(struct BMPHeader) + sizeof(struct BMPInfoHeader));

    if (this->infoHeader.bitsPerPixel < 8) {
        const uint8_t* curr = data + sizeof(this->header) + sizeof(this->infoHeader);
        for (size_t i = 0; i < this->infoHeader.colorsUsed; i++) {
            this->colorTable.colors[i] = Pixel(curr[0], curr[1], curr[2]);
            printf("R:%u G:%u B:%u\n", colorTable.colors[i].red, colorTable.colors[i].green, colorTable.colors[i].blue);
            curr += 4;
        }
    }
    this->newPixelData(data + this->header.offset, datalen - this->header.offset);
}

void BMPDEC::newPixelData(const uint8_t* data, size_t datalen)
{
    if (this->infoHeader.bitsPerPixel == 1) {
        for (size_t i = 0; i < datalen; i++) {
            for (int j = 0; j < 8; j++) {
                int px = i * 8 + j;
                size_t row = px / this->infoHeader.width;
                size_t col = px % this->infoHeader.width;
                uint8_t color = data[i] & (1 << (7 - j));

                this->rowData[col] = this->colorTable.colors[color];

                if (col >= this->infoHeader.width - 1) {
                    // image is inverted verticaly
                    this->callback(this->rowData, this->infoHeader.height - 1 - row);
                }
            }
        }
    } else if (this->infoHeader.bitsPerPixel == 4) {
        for (size_t i = 0; i < datalen; i++) {
            for (int j = 0; j < 2; j++) {
                int px = i * 2 + j;
                size_t row = px / this->infoHeader.width;
                size_t col = px % this->infoHeader.width;
                uint8_t color = (data[i] >> (4 * (1 -j))) & 0xF;

                this->rowData[col] = this->colorTable.colors[color];

                if (col >= this->infoHeader.width - 1) {
                    // image is inverted verticaly
                    this->callback(this->rowData, this->infoHeader.height - 1 - row);
                }
            }
        }
    }
    else if (this->infoHeader.bitsPerPixel == 24) {
        for (size_t px = 0; px < this->infoHeader.width * this->infoHeader.height * 3; px += 3) {
            size_t row = (px / 3) / this->infoHeader.width;
            size_t col = (px / 3) % this->infoHeader.width;

            uint8_t r = data[px];
            uint8_t g = data[px + 1];
            uint8_t b = data[px + 2];

            this->rowData[col] = Pixel(r, g, b);

            if (col >= this->infoHeader.width - 1) {
                // image is inverted verticaly
                this->callback(this->rowData, this->infoHeader.height - 1 - row);
            }
        }
    }
}
