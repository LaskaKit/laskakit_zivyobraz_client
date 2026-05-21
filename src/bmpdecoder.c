#include <string.h>
#include "bmpdecoder.h"


/* ── little-endian helpers ──────────────────────────────────────────────── */

static uint16_t read_u16le(const uint8_t* p) {
    return (uint16_t)(p[0] | ((uint16_t)p[1] << 8));
}

static uint32_t read_u32le(const uint8_t* p) {
    return (uint32_t)p[0]
         | ((uint32_t)p[1] <<  8)
         | ((uint32_t)p[2] << 16)
         | ((uint32_t)p[3] << 24);
}

static int32_t read_i32le(const uint8_t* p) {
    return (int32_t)read_u32le(p);
}

/* ── RLE4 sub-state constants ───────────────────────────────────────────── */

#define RLE4_CMD0   0  /* waiting for first byte of command */
#define RLE4_CMD1   1  /* waiting for second byte (count stored in rleCount) */
#define RLE4_ABS    2  /* reading absolute-mode pixel bytes */
#define RLE4_ABSPAD 3  /* consuming word-alignment padding byte */
#define RLE4_DELTAX 4  /* reading delta X */
#define RLE4_DELTAY 5  /* reading delta Y */

/* ── colour helpers ─────────────────────────────────────────────────────── */

/* Expand a 5-bit channel to 8 bits by replicating the top bits into the gap. */
static uint8_t expand5(uint8_t v5) { return (uint8_t)((v5 << 3) | (v5 >> 2)); }
/* Expand a 6-bit channel to 8 bits. */
static uint8_t expand6(uint8_t v6) { return (uint8_t)((v6 << 2) | (v6 >> 4)); }

uint8_t BMPNearestPaletteIndex(uint8_t r, uint8_t g, uint8_t b,
                               const uint16_t* palette, uint8_t paletteSize)
{
    uint8_t  best     = 0;
    uint32_t bestDist = UINT32_MAX;

    for (uint8_t i = 0; i < paletteSize; i++) {
        uint16_t c  = palette[i];
        uint8_t  pr = expand5((c >> 11) & 0x1F);
        uint8_t  pg = expand6((c >>  5) & 0x3F);
        uint8_t  pb = expand5( c        & 0x1F);

        int32_t dr = (int32_t)r - pr;
        int32_t dg = (int32_t)g - pg;
        int32_t db = (int32_t)b - pb;
        uint32_t dist = (uint32_t)(dr*dr + dg*dg + db*db);

        if (dist < bestDist) {
            bestDist = dist;
            best = i;
            if (dist == 0) break;
        }
    }
    return best;
}

/* ── header parsing ─────────────────────────────────────────────────────── */

static void parse_header(struct BMPDecoder* d)
{
    const uint8_t* h = d->headerBuf;

    if (h[0] != 'B' || h[1] != 'M') {
        d->state = BMP_ERROR;
        d->error = BMP_ERROR_SIGNATURE;
        return;
    }

    uint16_t bitsPerPixel = read_u16le(h + 28);
    uint32_t compression  = read_u32le(h + 30);

    uint8_t compressionOk =
        (compression == 0 && (bitsPerPixel == 24 || bitsPerPixel == 4)) ||
        (compression == 2 &&  bitsPerPixel == 4);
    if (!compressionOk) {
        d->state = BMP_ERROR;
        d->error = BMP_ERROR_FORMAT;
        return;
    }

    d->bitsPerPixel = (uint8_t)bitsPerPixel;
    d->compression  = (uint8_t)compression;

    uint32_t pixelDataOffset = read_u32le(h + 10);
    int32_t  imgHeight       = read_i32le(h + 22);
    uint32_t clrUsed         = read_u32le(h + 46);

    d->bottomUp   = (imgHeight > 0) ? 1 : 0;
    d->currentRow = d->bottomUp ? (uint16_t)(d->height - 1) : 0;

    if (bitsPerPixel == 4) {
        d->colorTableCount = (uint8_t)(clrUsed ? clrUsed : 16);
        if (compression == 0) {
            uint32_t rowBytes = ((uint32_t)d->width + 1) / 2;
            d->rowPadding     = (uint8_t)((4 - (rowBytes & 3)) & 3);
        } else {
            d->rowPadding = 0;  /* RLE4: row boundaries via EOL escape, no padding */
        }
    } else {
        d->colorTableCount = 0;
        uint32_t rowBytes  = (uint32_t)d->width * 3;
        d->rowPadding      = (uint8_t)((4 - (rowBytes & 3)) & 3);
    }

    d->bytesToSkip = (pixelDataOffset > BMP_HEADER_SIZE)
                   ? pixelDataOffset - BMP_HEADER_SIZE
                   : 0;

    d->state = (d->bytesToSkip > 0) ? BMP_SKIP : BMP_DATA;
}

/* ── shared row-complete helper ─────────────────────────────────────────── */

static void finish_row(struct BMPDecoder* d)
{
    d->currentCol  = 0;
    d->paddingLeft = d->rowPadding;

    if (d->rowCallback) d->rowCallback(d);

    if (d->bottomUp) {
        if (d->currentRow == 0) { d->state = BMP_DONE; return; }
        d->currentRow--;
    } else {
        d->currentRow++;
        if (d->currentRow >= d->height) d->state = BMP_DONE;
    }
}

/* ── 24 BPP pixel ───────────────────────────────────────────────────────── */

static void finish_pixel_24(struct BMPDecoder* d)
{
    uint8_t b = d->pixelBuf[0];
    uint8_t g = d->pixelBuf[1];
    uint8_t r = d->pixelBuf[2];

    d->rowBuffer[d->currentCol] =
        BMPNearestPaletteIndex(r, g, b, d->palette, d->paletteSize);
    d->currentCol++;

    if (d->currentCol >= d->width) finish_row(d);
}

/* ── 4 BPP nibble ───────────────────────────────────────────────────────── */

static void process_nibble4(struct BMPDecoder* d, uint8_t nibble)
{
    if (nibble >= d->colorTableCount) nibble = 0;

    uint8_t off = (uint8_t)(nibble * 4);  /* BGRA entry offset */
    uint8_t b   = d->colorTable[off + 0];
    uint8_t g   = d->colorTable[off + 1];
    uint8_t r   = d->colorTable[off + 2];

    d->rowBuffer[d->currentCol] = nibble;
        // BMPNearestPaletteIndex(r, g, b, d->palette, d->paletteSize);
    d->currentCol++;

    if (d->currentCol >= d->width) finish_row(d);
}

/* ── RLE4 helpers ───────────────────────────────────────────────────────── */

/* Write one colour-table entry to rowBuffer; does NOT call finish_row.
   Row boundaries are controlled exclusively by EOL/EOB/delta escapes. */
static void emit_pixel_rle4(struct BMPDecoder* d, uint8_t nibble)
{
    if (d->currentCol >= d->width) return;
    if (nibble >= d->colorTableCount) nibble = 0;
    d->rowBuffer[d->currentCol++] = nibble;
}

/* Fill any remaining columns with colour 0 and close the row. */
static void rle4_eol(struct BMPDecoder* d)
{
    while (d->currentCol < d->width)
        emit_pixel_rle4(d, 0);
    finish_row(d);
}

static void process_rle4_byte(struct BMPDecoder* d, uint8_t byte)
{
    switch (d->rle4State) {

    case RLE4_CMD0:
        d->rleCount  = byte;
        d->rle4State = RLE4_CMD1;
        break;

    case RLE4_CMD1:
        if (d->rleCount > 0) {
            /* Encoded run: rleCount pixels alternating between high and low nibble. */
            uint8_t hi = (byte >> 4) & 0x0F;
            uint8_t lo = byte & 0x0F;
            for (uint8_t p = 0; p < d->rleCount; p++) {
                emit_pixel_rle4(d, (p & 1) == 0 ? hi : lo);
                if (d->state != BMP_DATA) break;
            }
            d->rle4State = RLE4_CMD0;
        } else {
            /* Escape sequence. */
            if (byte == 0) {
                rle4_eol(d);
                d->rle4State = RLE4_CMD0;
            } else if (byte == 1) {
                d->state     = BMP_DONE;
                d->rle4State = RLE4_CMD0;
            } else if (byte == 2) {
                d->rle4State = RLE4_DELTAX;
            } else {
                /* Absolute mode: 'byte' literal pixels follow as packed nibbles.
                   Data bytes = ceil(byte/2); padded to a 2-byte (word) boundary. */
                d->rlePixLeft      = byte;
                d->rleAbsBytesLeft = (uint8_t)((byte + 1) / 2);
                d->rleAbsPad       = (d->rleAbsBytesLeft & 1) ? 1 : 0;
                d->rle4State       = RLE4_ABS;
            }
        }
        break;

    case RLE4_ABS:
        {
            uint8_t hi = (byte >> 4) & 0x0F;
            uint8_t lo = byte & 0x0F;
            if (d->rlePixLeft > 0) { emit_pixel_rle4(d, hi); d->rlePixLeft--; }
            if (d->rlePixLeft > 0) { emit_pixel_rle4(d, lo); d->rlePixLeft--; }
            if (--d->rleAbsBytesLeft == 0)
                d->rle4State = d->rleAbsPad ? RLE4_ABSPAD : RLE4_CMD0;
        }
        break;

    case RLE4_ABSPAD:
        /* Consume the word-alignment padding byte. */
        d->rle4State = RLE4_CMD0;
        break;

    case RLE4_DELTAX:
        d->rleDeltaX = byte;
        d->rle4State = RLE4_DELTAY;
        break;

    case RLE4_DELTAY:
        {
            uint8_t dx = d->rleDeltaX;
            uint8_t dy = byte;
            if (dy == 0) {
                /* Horizontal-only skip: fill dx pixels with colour 0. */
                for (uint8_t k = 0; k < dx; k++)
                    emit_pixel_rle4(d, 0);
            } else {
                /* Fill and close current row, emit (dy-1) zero rows, then dx pixels. */
                while (d->currentCol < d->width) emit_pixel_rle4(d, 0);
                finish_row(d);
                for (uint8_t k = 1; k < dy && d->state == BMP_DATA; k++) {
                    while (d->currentCol < d->width) emit_pixel_rle4(d, 0);
                    finish_row(d);
                }
                for (uint8_t k = 0; k < dx && d->state == BMP_DATA; k++)
                    emit_pixel_rle4(d, 0);
            }
            d->rle4State = RLE4_CMD0;
        }
        break;
    }
}

/* ── public API ─────────────────────────────────────────────────────────── */

struct BMPDecoder CreateBMPDecoder(uint16_t width, uint16_t height,
                                   uint8_t* rowBuffer,
                                   const uint16_t* palette, uint8_t paletteSize,
                                   BMPRowCallback rowCallback)
{
    return (struct BMPDecoder){
        .state            = BMP_HEADER,
        .error            = BMP_ERROR_NONE,
        .width            = width,
        .height           = height,
        .currentRow       = 0,
        .currentCol       = 0,
        .bitsPerPixel     = 0,
        .compression      = 0,
        .headerBuf        = {0},
        .headerBytesRead  = 0,
        .colorTable       = {0},
        .colorTableIdx    = 0,
        .colorTableCount  = 0,
        .bytesToSkip      = 0,
        .pixelBuf         = {0},
        .pixelBufIdx      = 0,
        .rowPadding       = 0,
        .paddingLeft      = 0,
        .bottomUp         = 0,
        .rle4State        = RLE4_CMD0,
        .rleCount         = 0,
        .rlePixLeft       = 0,
        .rleAbsBytesLeft  = 0,
        .rleAbsPad        = 0,
        .rleDeltaX        = 0,
        .rowBuffer        = rowBuffer,
        .palette          = palette,
        .paletteSize      = paletteSize,
        .rowCallback      = rowCallback,
    };
}

void DecodeBMP(struct BMPDecoder* d, const uint8_t* data, size_t datalen)
{
    for (size_t i = 0; i < datalen; i++) {
        switch (d->state) {

        case BMP_HEADER:
            d->headerBuf[d->headerBytesRead++] = data[i];
            if (d->headerBytesRead >= BMP_HEADER_SIZE) parse_header(d);
            break;

        case BMP_SKIP:
            /* Accumulate colour table bytes for 4 BPP; discard the rest. */
            if (d->colorTableIdx < sizeof(d->colorTable))
                d->colorTable[d->colorTableIdx++] = data[i];
            if (--d->bytesToSkip == 0) d->state = BMP_DATA;
            break;

        case BMP_DATA:
            if (d->paddingLeft > 0) {
                d->paddingLeft--;
            } else if (d->bitsPerPixel == 4 && d->compression == 2) {
                process_rle4_byte(d, data[i]);
            } else if (d->bitsPerPixel == 4) {
                /* Each byte carries two nibbles: high = left pixel, low = right.
                   For odd-width rows the low nibble of the last byte is filler
                   and must be skipped; this is detected by the row completing
                   on the high nibble (colBefore == width - 1). */
                uint16_t colBefore = d->currentCol;
                process_nibble4(d, (data[i] >> 4) & 0x0F);
                if (d->state == BMP_DATA && colBefore < (uint16_t)(d->width - 1))
                    process_nibble4(d, data[i] & 0x0F);
            } else {
                d->pixelBuf[d->pixelBufIdx++] = data[i];
                if (d->pixelBufIdx == 3) {
                    d->pixelBufIdx = 0;
                    finish_pixel_24(d);
                }
            }
            break;

        case BMP_DONE:
        case BMP_ERROR:
            return;
        }
    }
}
