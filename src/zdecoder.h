#ifndef ZDECODER_H
#define ZDECODER_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stddef.h>

/**
 * @defgroup zdecoder ZDecoder
 * @brief RLE-compressed Z-format image decoder.
 *
 * Z2 encodes pixels as 2-bit color indices (4 colors, 6-bit run length).
 * Z3 encodes pixels as 3-bit color indices (8 colors, 5-bit run length).
 * Both formats begin with a 2-byte ASCII header: "Z2" or "Z3".
 * @{
 */

/** Z2 color index → RGB565. Order: white, black, red, yellow. */
extern const uint16_t z2ColorToRGB565Lut[4];

/** Z2 grayscale index → RGB565. Order: white, black, light gray, dark gray. */
extern const uint16_t z2GrayscaleToRGB565Lut[4];

/** Z3 color index → RGB565. Order: white, black, red, yellow, green, blue, orange, purple. */
extern const uint16_t z3ColorToRGB565Lut[8];

/** Z3 grayscale index → RGB565. Eight evenly distributed gray shades. */
extern const uint16_t z3GrayscaleToRGB565Lut[8];

/**
 * @brief One RLE-encoded byte.
 *
 * The same byte is interpreted differently depending on the stream type:
 * - Z2: upper 2 bits are the color index, lower 6 bits are the run length.
 * - Z3: upper 3 bits are the color index, lower 5 bits are the run length.
 */
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

/** Selects the bit layout used to decode @ref RLEByte. */
enum ZDecoderType {
    Z2, /**< 2-bit color, 6-bit run length. */
    Z3  /**< 3-bit color, 5-bit run length. */
};

/**
 * @brief Decoder state machine state.
 *
 * The decoder starts in @c ZHEADER and transitions to @c ZDATA after
 * consuming the 2-byte header.  Any error transitions to @c ZERROR and
 * halts further processing.
 */
enum ZDecoderState {
    ZHEADER, /**< Waiting to consume the 2-byte "Z2"/"Z3" header. */
    ZDATA,   /**< Header consumed; decoding pixel data. */
    ZERROR,  /**< Unrecoverable error; see @ref ZDecoderError. */
};

/** Selects which built-in LUT to use for color conversion. */
enum ZColorSpace {
    ZGRAYSCALE, /**< Use the grayscale LUTs. */
    ZCOLOR,     /**< Use the color LUTs. */
};

/** Error codes stored in @ref ZDecoder.error when state is @c ZERROR. */
enum ZDecoderError {
  ZERROR_OVERFLOW,  /**< Decoded pixels exceeded the declared image dimensions. */
  ZERROR_HEADER,    /**< Header bytes did not match "Z2" or "Z3". */
  ZERROR_BUFFER,    /**< @ref ZDecoder.rowBuffer is NULL. */
  ZERROR_CALLBACK,  /**< @ref ZDecoder.rowCallback is NULL. */
};

/**
 * @brief Convert a Z color index to an RGB565 value using a lookup table.
 *
 * @param zColor  Color index from the Z stream.
 * @param lut     Pointer to an array of RGB565 values, one per color index.
 * @param lutlen  Number of entries in @p lut.
 * @return        The RGB565 value, or 0x7814 (purple) when @p zColor >= @p lutlen.
 *
 * The built-in LUTs (@ref z2ColorToRGB565Lut etc.) cover the standard palettes,
 * but a custom LUT can be supplied here when the display requires different values.
 */
uint16_t ZtoRGB565(uint8_t zColor, const uint16_t* lut, uint8_t lutlen);

struct ZDecoder;

/**
 * @brief Called by the decoder each time a complete image row has been written.
 *
 * @param decoder  Pointer to the decoder.  @c decoder->rowBuffer holds the
 *                 raw color indices for the completed row.  @c decoder->currentRow
 *                 is the zero-based index of the row that was just finished.
 *
 * The callback must not modify @c decoder->state or @c decoder->currentRow.
 * Convert indices to RGB565 with @ref ZtoRGB565 inside the callback.
 */
typedef void (*ZDecoderRowCallback)(const struct ZDecoder* decoder);

/**
 * @brief Decoder state.  Initialise with @ref CreateZDecoder.
 *
 * After calling @ref DecodeZ, check @c state:
 * - @c ZDATA — still in progress (stream may be split across multiple calls).
 * - @c ZERROR — decoding failed; @c error contains the reason.
 *
 * @c rowBuffer is owned by the caller and must remain valid for the lifetime
 * of the decoder.  It must be at least @c width bytes long.
 */
struct ZDecoder {
    enum ZDecoderType type;          /**< Detected from the header; do not set manually. */
    enum ZDecoderState state;        /**< Current state machine state. */
    enum ZDecoderError error;        /**< Valid only when @c state == @c ZERROR. */
    uint8_t header[2];               /**< Raw header bytes accumulated during @c ZHEADER state. */
    uint16_t width;                  /**< Image width in pixels. */
    uint16_t height;                 /**< Image height in pixels. */
    uint8_t* rowBuffer;              /**< Caller-supplied buffer; holds one row of color indices. */
    uint16_t currentRow;             /**< Zero-based index of the row currently being decoded. */
    uint16_t currentCol;             /**< Column position within the current row. */
    ZDecoderRowCallback rowCallback; /**< Invoked when a full row is ready in @c rowBuffer. */
};

/**
 * @brief Initialise a @ref ZDecoder.
 *
 * @param width        Image width in pixels.
 * @param height       Image height in pixels.
 * @param rowBuffer    Caller-allocated buffer of at least @p width bytes.
 * @param rowCallback  Function called after each complete row is decoded.
 * @return             A zero-initialised decoder ready to accept data.
 */
struct ZDecoder CreateZDecoder(uint16_t width, uint16_t height, uint8_t* rowBuffer, ZDecoderRowCallback rowCallback);

/**
 * @brief Decode a single RLE byte and write pixels to @c decoder->rowBuffer.
 *
 * Fires @c rowCallback whenever a row boundary is crossed.  Sets
 * @c decoder->state to @c ZERROR on pixel overflow.
 *
 * @param decoder  Decoder to update.
 * @param zbyte    One RLE byte from the Z data section (after the header).
 */
void DecodeZByte(struct ZDecoder* decoder, struct RLEByte zbyte);

/**
 * @brief Decode a chunk of Z-format data.
 *
 * The first call must include the 2-byte "Z2" or "Z3" header.  Subsequent
 * calls can pass arbitrary-size chunks; the decoder is fully incremental.
 *
 * @param decoder  Decoder to update.
 * @param data     Pointer to the data chunk.
 * @param datalen  Number of bytes in @p data.
 *
 * On return, check @c decoder->state.  @c ZERROR means decoding has stopped
 * and @c decoder->error identifies the cause.
 */
void DecodeZ(struct ZDecoder* decoder, const uint8_t* data, size_t datalen);

/** @} */

#ifdef __cplusplus
}
#endif

#ifdef __cplusplus

/**
 * @brief C++ wrapper around @ref ZDecoder.
 *
 * Constructs the underlying decoder and exposes @c decode(), @c state(),
 * and @c error() as member functions.  @c rowBuffer must remain valid for
 * the lifetime of this object.
 */
class ZDec
{
private:
public:
  ZDecoder _decoder;

  /**
   * @param width        Image width in pixels.
   * @param height       Image height in pixels.
   * @param rowBuffer    Caller-allocated buffer of at least @p width bytes.
   * @param rowCallback  Called after each complete row is written to @p rowBuffer.
   */
  ZDec(uint16_t width, uint16_t height, uint8_t* rowBuffer, ZDecoderRowCallback rowCallback)
    : _decoder(CreateZDecoder(width, height, rowBuffer, rowCallback))
  {}

  /**
   * @brief Decode a chunk of Z-format data.  May be called incrementally.
   * @param data     Pointer to the data chunk (must include the header on the first call).
   * @param datalen  Number of bytes in @p data.
   */
  void decode(const uint8_t* data, size_t datalen)
  {
    DecodeZ(&_decoder, data, datalen);
  }

  /** @return Current decoder state. */
  ZDecoderState state()
  {
    return _decoder.state;
  }

  /** @return Error code; valid only when state() == ZERROR. */
  ZDecoderError error()
  {
    return _decoder.error;
  }
};
#endif

#endif  // ZDECODER_H
