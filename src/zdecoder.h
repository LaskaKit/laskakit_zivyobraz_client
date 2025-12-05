#ifndef ZDECODER_H
#define ZDECODER_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stddef.h>

struct RLEByte {
  union {
    uint8_t raw;
    struct {
      uint8_t z2count : 6;
      uint8_t z2color : 2;
    };
    struct {
      uint8_t z3count : 5;
      uint8_t z3color : 3;
    };
  };
};

enum ZDecoderType {
    Z2,
    Z3
};

enum ZDecoderState {
    ZHEADER,
    ZDATA,
    ZERROR,
};

enum ZColorSpace {
    ZGRAYSCALE,
    ZCOLOR,
};

enum ZDecoderError {
  ZERROR_OVERFLOW,
  ZERROR_HEADER,
  ZERROR_BUFFER,
  ZERROR_CALLBACK,
};

uint16_t ZtoRGB565(uint8_t zColor, const uint16_t* lut, uint8_t lutlen);

struct ZDecoder;
typedef void (*ZDecoderRowCallback)(const struct ZDecoder* decoder);

struct ZDecoder {
    enum ZDecoderType type;
    enum ZDecoderState state;
    enum ZDecoderError error;
    uint8_t header[2];
    uint16_t width;
    uint16_t height;

    uint8_t* rowBuffer;

    uint16_t currentRow;
    uint16_t currentCol;

    ZDecoderRowCallback rowCallback;
};

struct ZDecoder CreateZDecoder(uint16_t width, uint16_t height, uint8_t* rowBuffer, ZDecoderRowCallback rowCallback);
void DecodeZByte(struct ZDecoder* decoder, struct RLEByte zbyte);
void DecodeZ(struct ZDecoder* decoder, const uint8_t* data, size_t datalen);

#ifdef __cplusplus
}
#endif

#ifdef __cplusplus

class ZDec
{
private:
public:
  ZDecoder _decoder;
  ZDec(uint16_t width, uint16_t height, uint8_t* rowBuffer, ZDecoderRowCallback rowCallback)
    : _decoder(CreateZDecoder(width, height, rowBuffer, rowCallback))
  {}
  void decode(const uint8_t* data, size_t datalen)
  {
    DecodeZ(&_decoder, data, datalen);
  }
  ZDecoderState state()
  {
    return _decoder.state;
  }
  ZDecoderError error()
  {
    return _decoder.error;
  }
};
#endif

#endif  // ZDECODER_H
