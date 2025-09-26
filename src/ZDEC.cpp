
#include <cstdio>

#include "ZDEC.hpp"

using namespace LaskaKit::ZivyObraz;

void ZDEC::newData(const uint8_t* data, size_t datalen)
{
    this->newPixelData(data + 2, datalen - 2);
}

void ZDEC::newPixelData(const uint8_t* data, size_t datalen)
{
    size_t px = 0;
    if (this->type == 1) {

    } else if (this->type == 2) {
        for (size_t i = 0; i < datalen; i++) {
            uint8_t color = data[i] >> 6;
            uint8_t count = data[i] & (0xFF >> 2);

            for (int j = 0; j < count; j++) {
                int row = px / this->width;
                int col = px % this->width;

                switch (color) {
                    case 0:  // white
                        this->rowData[col] = Pixel(0xFF, 0xFF, 0xFF);
                        break;
                    case 1:  // black
                        this->rowData[col] = Pixel(0x00, 0x00, 0x00);
                        break;
                    case 2:  // light gray or red (use red)
                        this->rowData[col] = Pixel(0xC0, 0xC0, 0xC0);
                        break;
                    case 3:  // dark gray or yellow (use yellow)
                        this->rowData[col] = Pixel(0x80, 0x80, 0x80);
                        break;
                }

                if (col >= this->width - 1) {
                    this->callback(this->rowData, row);
                }
                px++;
            }
        }
    } else if (this->type == 3) {

    }
}

void StreamingZDEC::reset()
{
    this->currentCol = 0;
    this->currentRow = 0;
}

void StreamingZDEC::newData(const uint8_t* data, size_t datalen)
{
    if (this->currentHeader >= 2) {
        this->newPixelData(data, datalen);
    } else {
        this->newHeaderData(data, datalen);
    }
}

void StreamingZDEC::newHeaderData(const uint8_t* data, size_t datalen)
{
    this->currentHeader = 2;
    this->newPixelData(data + 2, datalen -2);
}

void StreamingZDEC::newPixelData(const uint8_t* data, size_t datalen)
{
    for (size_t i = 0; i < datalen; i++) {
        this->processRLEByte(data[i]);
    }
}

void StreamingZDEC::processRLEByte(uint8_t byte)
{
    if (this->type == 1) {

    } else if (this->type == 2) {
        uint8_t color = byte >> 6;
        uint8_t count = byte & (0xFF >> 2);

        for (int j = 0; j < count; j++) {
            switch (color) {
                case 0:  // white
                    this->rowData[this->currentCol] = Pixel(0xFF, 0xFF, 0xFF);
                    break;
                case 1:  // black
                    this->rowData[this->currentCol] = Pixel(0x00, 0x00, 0x00);
                    break;
                case 2:  // light gray or red (use light gray)
                    this->rowData[this->currentCol] = Pixel(0xC0, 0xC0, 0xC0);
                    break;
                case 3:  // dark gray or yellow (use yellow)
                    this->rowData[this->currentCol] = Pixel(0x80, 0x80, 0x80);
                    break;
            }
            this->currentCol++;

            if (this->currentCol >= this->width) {
                this->callback(this->rowData, this->currentRow);
                this->currentCol = 0;
                this->currentRow++;
            }
        }
    } else if (this->type == 3) {

    }
}
