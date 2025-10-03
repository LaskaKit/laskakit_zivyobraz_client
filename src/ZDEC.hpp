#include <cstdlib>
#include <cstdint>

#include "zivyobrazclient.hpp"

namespace LaskaKit::ZivyObraz {
    constexpr size_t Z_MAX_ROW_SIZE = 800;

    // class for decoding Zx formats
    class ZDEC
    {
    private:
        uint16_t width;
        uint16_t height;
        uint8_t type;  // 1/2/3
        Pixel rowData[800];
        ZIVYOBRAZ_DRAW_CALLBACK callback;

    public:
        ZDEC(uint16_t width, uint16_t height, uint8_t type)
            : width(width), height(height), type(type)
        {}
        void newData(const uint8_t* data, size_t datalen);
        void newPixelData(const uint8_t* data, size_t datalen);
        void addCallback(ZIVYOBRAZ_DRAW_CALLBACK callback)
        {
            this->callback = callback;
        }
    };

    class StreamingZDEC
    {
    private:
        uint16_t width;
        uint16_t height;
        uint8_t type;  // 1/2/3
        Pixel rowData[Z_MAX_ROW_SIZE];
        uint16_t currentRow;
        uint16_t currentCol;
        uint8_t currentHeader;
        ZIVYOBRAZ_DRAW_CALLBACK callback;

    public:
        StreamingZDEC() = default;
        StreamingZDEC(uint16_t width, uint16_t height, uint8_t type)
            : width(width), height(height), type(type), currentRow(0), currentCol(0), currentHeader(0)
        {
        }

        virtual ~StreamingZDEC()
        {
        }

        void reset();
        void newData(const uint8_t* data, size_t datalen);
        void newHeaderData(const uint8_t* data, size_t datalen);
        void newPixelData(const uint8_t* data, size_t datalen);
        void addCallback(ZIVYOBRAZ_DRAW_CALLBACK callback)
        {
            this->callback = callback;
        }

    private:
        void processRLEByte(uint8_t byte);
    };
}
