#include <stdint.h>

#include "zdecoder.h"
#include <stdio.h>


struct ZDecoder CreateZDecoder(uint16_t width, uint16_t height, uint8_t* rowBuffer, ZDecoderRowCallback rowCallback)
{
    return (struct ZDecoder){
        .currentCol = 0,
        .currentRow = 0,
        .state = ZHEADER,
        .type = Z2,
        .header = {0},
        .width = width,
        .height = height,
        .rowBuffer = rowBuffer,
        .rowCallback = rowCallback,
    };
}


void DecodeZByte(struct ZDecoder* decoder, struct RLEByte zbyte)
{
    uint8_t zcolor = 0, zcount = 0;
    switch (decoder->type) {
        case Z2:
            zcolor = zbyte.z2color;
            zcount = zbyte.z2count;
            break;
        case Z3:
            zcolor = zbyte.z3color;
            zcount = zbyte.z3count;
            break;
    }
    // printf("Count: %d\n", zcount);
    for (uint8_t c = 0; c < zcount; c++) {
        if (decoder->currentRow >= decoder->height) {
            decoder->state = ZERROR;  // overflow
            return;
        }
        decoder->rowBuffer[decoder->currentCol] = zcolor;
        decoder->currentCol++;

        // check if we are at the end of the row
        if (decoder->currentCol >= decoder->width) {
            decoder->rowCallback(decoder);
            decoder->currentCol = 0;
            decoder->currentRow++;
        }
    }
}

void DecodeZ(struct ZDecoder* decoder, const uint8_t* data, size_t datalen)
{
    if (decoder->rowBuffer == NULL) {
        decoder->state = ZERROR;
        decoder->error = ZERROR_BUFFER;
        return;
    }
    if (decoder->rowCallback == NULL ) {
        decoder->state = ZERROR;
        decoder->error = ZERROR_CALLBACK;
        return;
    }

    size_t processedData = 0;
    while (processedData < datalen) {
        switch (decoder->state) {
            case ZERROR:
                return;
            case ZHEADER:
                decoder->header[processedData] = data[processedData];
                processedData++;

                if (processedData == 2) {
                    if (decoder->header[0] == 'Z' && decoder->header[1] == '2') {
                        decoder->type = Z2;
                    } else if (decoder->header[0] == 'Z' && decoder->header[1] == '3') {
                        decoder->type = Z3;
                    } else {
                        decoder->state = ZERROR;
                        decoder->error = ZERROR_HEADER;
                        return;
                    }
                    // header processed, now check
                    decoder->state = ZDATA;
                }
                break;
            case ZDATA:
                {
                    while(processedData < datalen && decoder->state != ZERROR) {
                        DecodeZByte(decoder, (struct RLEByte){.raw = data[processedData]});
                        processedData++;
                    }
                }
                break;
        }
    }
}
