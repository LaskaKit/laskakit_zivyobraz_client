#ifndef BMPDECODER_H
#define BMPDECODER_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stddef.h>

/**
 * @defgroup bmpdecoder BMPDecoder
 * @brief Uncompressed BMP image decoder (4 BPP and 24 BPP) with nearest-palette-colour mapping.
 *
 * Incremental streaming decoder — @ref DecodeBMP may be called with
 * arbitrarily-sized chunks.  Each completed row is written as palette
 * colour indices into @c rowBuffer and @c rowCallback is fired.
 *
 * Supports both bottom-up (positive height, standard) and top-down
 * (negative height) BMP orientation; @c currentRow in the callback is
 * always the display row (0 = top).
 * @{
 */

enum BMPDecoderState {
    BMP_HEADER, /**< Accumulating the 54-byte file + DIB header. */
    BMP_SKIP,   /**< Skipping extra bytes between header and pixel data. */
    BMP_DATA,   /**< Decoding pixel rows. */
    BMP_DONE,   /**< All rows decoded successfully. */
    BMP_ERROR,  /**< Unrecoverable error; see @ref BMPDecoderError. */
};

enum BMPDecoderError {
    BMP_ERROR_NONE,       /**< No error. */
    BMP_ERROR_SIGNATURE,  /**< Missing "BM" magic bytes. */
    BMP_ERROR_FORMAT,     /**< Unsupported format (not 24-bit uncompressed). */
};

struct BMPDecoder;

/**
 * @brief Called by the decoder each time a complete image row has been written.
 *
 * @param decoder  @c decoder->rowBuffer holds palette indices for the row.
 *                 @c decoder->currentRow is the zero-based display row (0 = top).
 *
 * The callback must not modify @c state or @c currentRow.
 */
typedef void (*BMPRowCallback)(const struct BMPDecoder* decoder);

#define BMP_FILE_HEADER_SIZE 14
#define BMP_DIB_HEADER_SIZE  40
#define BMP_HEADER_SIZE      (BMP_FILE_HEADER_SIZE + BMP_DIB_HEADER_SIZE)

/**
 * @brief Decoder state.  Initialise with @ref CreateBMPDecoder.
 */
struct BMPDecoder {
    enum BMPDecoderState state;
    enum BMPDecoderError error;

    uint16_t width;          /**< Expected image width in pixels (set by caller). */
    uint16_t height;         /**< Expected image height in pixels (set by caller). */
    uint16_t currentRow;     /**< Current display row (0 = top). */
    uint16_t currentCol;     /**< Current column within the row. */
    uint8_t  bitsPerPixel;   /**< 4 or 24; parsed from the DIB header. */

    /* header parsing */
    uint8_t  headerBuf[BMP_HEADER_SIZE];
    uint8_t  headerBytesRead;

    /* 4 BPP colour table (BGRA, up to 16 entries = 64 bytes).
       Read from the bytes between the DIB header and pixel data. */
    uint8_t  colorTable[64];
    uint8_t  colorTableIdx;    /**< Bytes written into colorTable so far. */
    uint8_t  colorTableCount;  /**< Number of entries expected. */

    /* pixel data state */
    uint32_t bytesToSkip;   /**< Extra bytes between header and pixel data. */
    uint8_t  pixelBuf[3];   /**< Accumulates BGR bytes for one 24 BPP pixel. */
    uint8_t  pixelBufIdx;
    uint8_t  rowPadding;    /**< Padding bytes appended to each BMP row. */
    uint8_t  paddingLeft;   /**< Padding bytes remaining in the current row. */
    uint8_t  bottomUp;      /**< 1 if BMP rows are stored bottom-to-top. */

    /* output */
    uint8_t*        rowBuffer;    /**< Caller-supplied buffer; at least @c width bytes. */
    const uint16_t* palette;      /**< RGB565 palette used for nearest-colour mapping. */
    uint8_t         paletteSize;
    BMPRowCallback  rowCallback;
};

/**
 * @brief Find the palette index whose RGB565 colour is closest to (r, g, b).
 *
 * Distance is the squared Euclidean distance in RGB888 space.
 *
 * @param r            Red component (0–255).
 * @param g            Green component (0–255).
 * @param b            Blue component (0–255).
 * @param palette      Array of RGB565 values.
 * @param paletteSize  Number of entries in @p palette.
 * @return             Index of the nearest palette entry.
 */
uint8_t BMPNearestPaletteIndex(uint8_t r, uint8_t g, uint8_t b,
                               const uint16_t* palette, uint8_t paletteSize);

/**
 * @brief Initialise a @ref BMPDecoder.
 *
 * @param width        Expected image width in pixels.
 * @param height       Expected image height in pixels.
 * @param rowBuffer    Caller-allocated buffer of at least @p width bytes.
 * @param palette      RGB565 palette for nearest-colour mapping.
 * @param paletteSize  Number of entries in @p palette.
 * @param rowCallback  Function called after each complete row is decoded.
 * @return             A zero-initialised decoder ready to accept data.
 */
struct BMPDecoder CreateBMPDecoder(uint16_t width, uint16_t height,
                                   uint8_t* rowBuffer,
                                   const uint16_t* palette, uint8_t paletteSize,
                                   BMPRowCallback rowCallback);

/**
 * @brief Decode a chunk of BMP data.
 *
 * The first call must begin at the start of the BMP file (the "BM" signature).
 * Subsequent calls may pass arbitrary-size chunks; the decoder is fully
 * incremental.
 *
 * @param decoder  Decoder to update.
 * @param data     Pointer to the data chunk.
 * @param datalen  Number of bytes in @p data.
 */
void DecodeBMP(struct BMPDecoder* decoder, const uint8_t* data, size_t datalen);

/** @} */

#ifdef __cplusplus
}
#endif


#ifdef __cplusplus

/**
 * @brief C++ wrapper around @ref BMPDecoder.
 */
class BMPDec
{
public:
    struct BMPDecoder _decoder;

    BMPDec() : _decoder{} {}

    /**
     * @param width        Image width in pixels.
     * @param height       Image height in pixels.
     * @param rowBuffer    Caller-allocated buffer of at least @p width bytes.
     * @param palette      RGB565 palette for nearest-colour mapping.
     * @param paletteSize  Number of entries in @p palette.
     * @param rowCallback  Called after each complete row is written to @p rowBuffer.
     */
    void init(uint16_t width, uint16_t height,
              uint8_t* rowBuffer,
              const uint16_t* palette, uint8_t paletteSize,
              BMPRowCallback rowCallback)
    {
        _decoder = CreateBMPDecoder(width, height, rowBuffer, palette, paletteSize, rowCallback);
    }

    /** @brief Decode a chunk of BMP data.  May be called incrementally. */
    void decode(const uint8_t* data, size_t datalen)
    {
        DecodeBMP(&_decoder, data, datalen);
    }

    /** @return Current decoder state. */
    BMPDecoderState state() { return _decoder.state; }

    /** @return Error code; valid only when state() == BMP_ERROR. */
    BMPDecoderError error() { return _decoder.error; }
};

#endif  // __cplusplus

#endif  // BMPDECODER_H
