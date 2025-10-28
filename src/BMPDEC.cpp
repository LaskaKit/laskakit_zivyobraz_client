// #include <cstring>
// #include <cstdlib>
// #include <cstring>
// #include <cstdio>
// #include <memory>

// #include "BMPDEC.hpp"

// using namespace LaskaKit::ZivyObraz;

// void BMPDEC::newData(const uint8_t* data, size_t datalen)
// {
//     if (!this->headerRead) {
//         memcpy(&this->header, data, sizeof(this->header));
//         this->headerRead = true;
//     }

//     if (!this->infoHeaderRead) {
//         memcpy(&this->infoHeader, data + sizeof(this->header), sizeof(this->infoHeader));
//         this->infoHeaderRead = true;
//     }

//     printf("File type: %c%c\n", header.type, header.type >> 8);
//     printf("File size: %u\n", header.size);
//     printf("Offset: %u\n", header.offset);

//     printf("Header size: %u\n", infoHeader.headerSize);
//     printf("Width: %u\n", infoHeader.width);
//     printf("Height: %u\n", infoHeader.height);
//     printf("Planes: %u\n", infoHeader.planes);
//     printf("Bits per pixel: %u\n", infoHeader.bitsPerPixel);
//     printf("Compression: %u\n", infoHeader.compression);
//     printf("Image size: %u\n", infoHeader.imageSize);
//     printf("X pixels per m: %u\n", infoHeader.xPixelsPerM);
//     printf("Y pixels per m: %u\n", infoHeader.yPixelsPerM);
//     printf("Colors used: %u\n", infoHeader.colorsUsed);
//     printf("Important colors: %u\n", infoHeader.importantColors);
//     printf("Read so far: %ld\n", sizeof(struct BMPHeader) + sizeof(struct BMPInfoHeader));

//     if (this->infoHeader.bitsPerPixel < 8) {
//         const uint8_t* curr = data + sizeof(this->header) + sizeof(this->infoHeader);
//         for (size_t i = 0; i < this->infoHeader.colorsUsed; i++) {
//             this->colorTable.colors[i] = Pixel(curr[0], curr[1], curr[2]);
//             printf("R:%u G:%u B:%u\n", colorTable.colors[i].red, colorTable.colors[i].green, colorTable.colors[i].blue);
//             curr += 4;
//         }
//     }
//     this->newPixelData(data + this->header.offset, datalen - this->header.offset);
// }

// void BMPDEC::newPixelData(const uint8_t* data, size_t datalen)
// {
//     if (this->infoHeader.bitsPerPixel == 1) {
//         for (size_t i = 0; i < datalen; i++) {
//             for (int j = 0; j < 8; j++) {
//                 int px = i * 8 + j;
//                 size_t row = px / this->infoHeader.width;
//                 size_t col = px % this->infoHeader.width;
//                 uint8_t color = data[i] & (1 << (7 - j));

//                 this->rowData[col] = this->colorTable.colors[color];

//                 if (col >= this->infoHeader.width - 1) {
//                     // image is inverted verticaly
//                     this->callback(this->rowData, this->infoHeader.height - 1 - row);
//                 }
//             }
//         }
//     } else if (this->infoHeader.bitsPerPixel == 4) {
//         for (size_t i = 0; i < datalen; i++) {
//             for (int j = 0; j < 2; j++) {
//                 int px = i * 2 + j;
//                 size_t row = px / this->infoHeader.width;
//                 size_t col = px % this->infoHeader.width;
//                 uint8_t color = (data[i] >> (4 * (1 -j))) & 0xF;

//                 this->rowData[col] = this->colorTable.colors[color];

//                 if (col >= this->infoHeader.width - 1) {
//                     // image is inverted verticaly
//                     this->callback(this->rowData, this->infoHeader.height - 1 - row);
//                 }
//             }
//         }
//     }
//     else if (this->infoHeader.bitsPerPixel == 24) {
//         for (size_t px = 0; px < this->infoHeader.width * this->infoHeader.height * 3; px += 3) {
//             size_t row = (px / 3) / this->infoHeader.width;
//             size_t col = (px / 3) % this->infoHeader.width;

//             uint8_t r = data[px];
//             uint8_t g = data[px + 1];
//             uint8_t b = data[px + 2];

//             this->rowData[col] = Pixel(r, g, b);

//             if (col >= this->infoHeader.width - 1) {
//                 // image is inverted verticaly
//                 this->callback(this->rowData, this->infoHeader.height - 1 - row);
//             }
//         }
//     }
// }


// void StreamingBMPDEC::newData(const uint8_t* data, size_t datalen)
// {
//     if (!this->headersLoaded) {
//         this->newHeaderData(data, datalen);
//     } else {
//         this->newPixelData(data, datalen);
//     }
// }

// void StreamingBMPDEC::newHeaderData(const uint8_t* data, size_t datalen)
// {
//     memcpy(&this->header, data, sizeof(this->header));
//     memcpy(&this->infoHeader, data + sizeof(this->header), sizeof(this->infoHeader));

//     printf("File type: %c%c\n", header.type, header.type >> 8);
//     printf("File size: %u\n", header.size);
//     printf("Offset: %u\n", header.offset);

//     printf("Header size: %u\n", infoHeader.headerSize);
//     printf("Width: %u\n", infoHeader.width);
//     printf("Height: %u\n", infoHeader.height);
//     printf("Planes: %u\n", infoHeader.planes);
//     printf("Bits per pixel: %u\n", infoHeader.bitsPerPixel);
//     printf("Compression: %u\n", infoHeader.compression);
//     printf("Image size: %u\n", infoHeader.imageSize);
//     printf("X pixels per m: %u\n", infoHeader.xPixelsPerM);
//     printf("Y pixels per m: %u\n", infoHeader.yPixelsPerM);
//     printf("Colors used: %u\n", infoHeader.colorsUsed);
//     printf("Important colors: %u\n", infoHeader.importantColors);
//     printf("Read so far: %ld\n", sizeof(struct BMPHeader) + sizeof(struct BMPInfoHeader));

//     size_t totalRead = sizeof(this->header) + sizeof(this->infoHeader);

//     if (this->infoHeader.bitsPerPixel < 8) {
//         const uint8_t* curr = data + sizeof(this->header) + sizeof(this->infoHeader);
//         for (size_t i = 0; i < this->infoHeader.colorsUsed; i++) {
//             this->colorTable.colors[i] = Pixel(curr[0], curr[1], curr[2]);
//             // printf("R:%u G:%u B:%u\n", colorTable.colors[i].red, colorTable.colors[i].green, colorTable.colors[i].blue);
//             curr += 4;
//             totalRead += 4;
//         }
//     }

//     this->headersLoaded = true;
//     this->newPixelData(data + totalRead, datalen - totalRead);
// }


// void StreamingBMPDEC::newPixelData(const uint8_t* data, size_t datalen)
// {
//     if (this->infoHeader.bitsPerPixel == 1) {
//         for (size_t i = 0; i < datalen; i++) {
//             for (int j = 0; j < 8; j++) {
//                 uint8_t color = data[i] & (1 << (7 - j));

//                 this->rowData[this->currentCol] = this->colorTable.colors[color];

//                 this->currentCol++;
//                 if (this->currentCol >= this->infoHeader.width) {
//                     // image is inverted verticaly
//                     this->callback(this->rowData, this->infoHeader.height - 1 - this->currentRow);
//                     this->currentCol = 0;
//                     this->currentRow = 0;
//                 }
//             }
//         }
//     } else if (this->infoHeader.bitsPerPixel == 4) {
//         for (size_t i = 0; i < datalen; i++) {
//             for (int j = 0; j < 2; j++) {
//                 // int px = i * 2 + j;
//                 // size_t row = px / this->infoHeader.width;
//                 // size_t col = px % this->infoHeader.width;
//                 uint8_t color = (data[i] >> (4 * (1 -j))) & 0xF;

//                 // Bounds checking for row buffer access
//                 if (this->currentCol >= BMP_MAX_ROW_SIZE) {
//                     printf("ERROR: currentCol %u exceeds buffer size %lu\n",
//                                  this->currentCol, BMP_MAX_ROW_SIZE);
//                     return;
//                 }
//                 if (color >= 16) {
//                     printf("ERROR: Color index %u exceeds color table size (16)\n", color);
//                     return;
//                 }

//                 this->rowData[this->currentCol] = this->colorTable.colors[color];

//                 this->currentCol++;
//                 if (this->currentCol >= this->infoHeader.width) {
//                     // Bounds checking for callback parameters
//                     if (this->currentRow >= this->infoHeader.height) {
//                         printf("ERROR: currentRow %u exceeds image height %u\n",
//                                      this->currentRow, this->infoHeader.height);
//                         return;
//                     }

//                     uint16_t callbackRow = this->infoHeader.height - 1 - this->currentRow;
//                     if (callbackRow >= 480) {
//                         printf("ERROR: Callback row %u exceeds display height (480)\n", callbackRow);
//                         return;
//                     }

//                     // image is inverted verticaly
//                     this->callback(this->rowData, callbackRow);
//                     this->currentCol = 0;
//                     this->currentRow++;
//                 }
//             }
//         }
//     }
//     else if (this->infoHeader.bitsPerPixel == 24) {
//         for (size_t i = 0; i < datalen; i++) {
//             switch (this->bufferedPixelIdx) {
//                 case 0:
//                     this->bufferedPixel.red = data[i];
//                     break;
//                 case 1:
//                     this->bufferedPixel.green = data[i];
//                     break;
//                 case 2:
//                     this->bufferedPixel.blue = data[i];
//             }
//             this->bufferedPixelIdx++;
//             if (this->bufferedPixelIdx < 3) {
//                 continue;
//             }

//             // flush the buffer
//             this->bufferedPixelIdx = 0;
//             this->rowData[this->currentCol] = this->bufferedPixel;

//             this->currentCol++;
//             if (this->currentCol >= this->infoHeader.width) {
//                 // Bounds checking for callback parameters
//                 if (this->currentRow >= this->infoHeader.height) {
//                     printf("ERROR: currentRow %u exceeds image height %u\n",
//                                     this->currentRow, this->infoHeader.height);
//                     return;
//                 }

//                 uint16_t callbackRow = this->infoHeader.height - 1 - this->currentRow;
//                 if (callbackRow >= 480) {
//                     printf("ERROR: Callback row %u exceeds display height (480)\n", callbackRow);
//                     return;
//                 }

//                 // image is inverted vertically
//                 this->callback(this->rowData, this->infoHeader.height - 1 - this->currentRow);
//                 this->currentCol = 0;
//                 this->currentRow++;
//             }
//         }
//     }
// }
