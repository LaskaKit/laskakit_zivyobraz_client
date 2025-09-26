#pragma once

#include <cstdint>
#include <cstring>
#include <functional>

namespace LaskaKit::ZivyObraz {
    constexpr size_t MAX_PARAMS = 15;
    constexpr size_t MAX_VALUE_LEN = 20;
    constexpr size_t MAX_KEY_LEN = 20;


    struct Pixel
    {
      uint8_t red;
      uint8_t green;
      uint8_t blue;

      Pixel(uint8_t red = 0, uint8_t green = 0, uint8_t blue = 0)
        : red(red), green(green), blue(blue)
      {}
    };


    using ZIVYOBRAZ_DRAW_CALLBACK = std::function<void(Pixel* rowData, uint16_t row)>;


    struct HttpParam
    {
        char key[MAX_KEY_LEN];
        char value[MAX_VALUE_LEN];
    };

    struct HttpParams
    {
      HttpParam params[MAX_PARAMS];
      size_t count = 0;

      bool add(const char* key, const char* value);
      size_t queryStrLength() const;
      void buildQuery(char* buffer, size_t buflen) const;
    };
}
