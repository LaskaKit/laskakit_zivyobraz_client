#pragma once

#include <cstdint>
#include <cstdio>
#include <functional>
#include <initializer_list>

// #include "zivyobrazclient.hpp"

namespace LaskaKit::ZivyObraz {
enum class ZCompressionType { Z2, Z3 };

enum class ZColorType {
  BW = 1,  // black/white
  G4 = 2,  // four levels of gray
  C4 = 3,  // four colors
  RBW = 4, // red/black/white
  YBW = 5, // yellow/black/white
  G8 = 6,  // eight levels of gray
  C7 = 7,  // seven colors
};

enum class ZColor {
  Black = 0,
  White = 1,
  Red = 2,
  Yellow = 3,
  LightGray = 4,
  DarkGray = 5,
};

template <class T> struct ZPalette {
  std::size_t m_size = 0;
  T m_palette[8]{};

  constexpr ZPalette<T>() = default;

  ZPalette<T>(T *palette, std::size_t size)
      : m_size(size > 8 ? 8 : size) {
    std::copy(palette, palette + m_size, m_palette);
  }

  ZPalette<T>(std::initializer_list<T> init)
      : m_size(init.size() > 8 ? 8 : init.size()) {
    std::copy(init.begin(), init.begin() + m_size, m_palette);
  }

  constexpr const T &operator[](std::size_t index) const {
    return this->m_palette[index];
  }

  constexpr std::size_t size() const { return m_size; }
};

ZPalette<ZColor> makeStandardPalette(ZColorType colorType) {
  switch (colorType) {
  case ZColorType::BW:
    return ZPalette<ZColor>({ZColor::White, ZColor::Black});
  case ZColorType::G4:
    return ZPalette<ZColor>(
        {ZColor::White, ZColor::Black, ZColor::LightGray, ZColor::DarkGray});
  case ZColorType::C4:
    return ZPalette<ZColor>(
        {ZColor::White, ZColor::Black, ZColor::Red, ZColor::Yellow});
  case ZColorType::RBW:
    return ZPalette<ZColor>(
        {ZColor::White, ZColor::Black, ZColor::Red, ZColor::Red});
  case ZColorType::YBW:
    return ZPalette<ZColor>(
        {ZColor::White, ZColor::Black, ZColor::Yellow, ZColor::Yellow});
    // TODO
    // case ZColorType::G8:
    // case ZColorType::C7:
  }
  return ZPalette<ZColor>();
}

enum class ZDECState { OK = 0, ERROR = 1, FINISHED = 2 };

template <ZCompressionType Compression, class T_COLOR> class StreamingZDEC {
private:
  uint16_t width;
  uint16_t height;

  ZPalette<T_COLOR> palette;
  uint16_t currentRow = 0;
  uint16_t currentCol = 0;
  ZDECState currentState = ZDECState::OK;
  char header[2] = {'\0', '\0'};
  bool headerParsed = false;

  T_COLOR *decodeBuffer = nullptr;
  std::size_t decodeBufferLength = 0;

  using ZDEC_ROW_CALLBACK =
      std::function<void(T_COLOR *decodedRow, uint16_t rowIdx)>;
  ZDEC_ROW_CALLBACK rowCallback;

public:
  StreamingZDEC(uint16_t width, uint16_t height)
      : width(width), height(height) {}

  virtual ~StreamingZDEC() {}

  void setDecodeBuffer(T_COLOR *decodeBuffer, std::size_t decodeBufferLength) {
    this->decodeBuffer = decodeBuffer;
    this->decodeBufferLength = decodeBufferLength;
    if (decodeBufferLength < this->width) {
      // buffer too small
      this->currentState = ZDECState::ERROR;
    }
  }

  void setRowCallback(ZDEC_ROW_CALLBACK rowCallback) {
    this->rowCallback = rowCallback;
  }

  void setPalette(ZPalette<T_COLOR> palette) { this->palette = palette; }

  void reset() {
    this->currentCol = 0;
    this->currentRow = 0;
    this->headerParsed = false;
    this->header[0] = '\0';
    this->header[1] = '\0';
    this->currentState = ZDECState::OK;
  }

  ZDECState getState() const { return this->currentState; }

  void decode(const uint8_t *data, std::size_t datalen) {
    if (this->palette.size() == 0) {
      // using default palette with zero elements
      this->currentState = ZDECState::ERROR;
    }

    if (!this->rowCallback) {
      // forgot to set callback
      this->currentState = ZDECState::ERROR;
    }

    if (this->decodeBuffer == nullptr) {
      // forgot to set decode buffer
      this->currentState = ZDECState::ERROR;
    }

    if (this->currentState == ZDECState::FINISHED) {
      // more data than expected or forgot to reset
      this->currentState = ZDECState::ERROR;
    }

    if (this->currentState == ZDECState::ERROR) {
      return;
    }

    if (this->headerParsed) {
      this->decodePixels(data, datalen);
    } else {
      this->decodeHeader(data, datalen);
    }
  }

private:
  void decodeHeader(const uint8_t *data, std::size_t datalen) {
    if (datalen == 0) {
      return;
    }
    if (datalen == 1) {
      if (this->header[0] == '\0') {
        this->header[0] = data[0];
        return;
      }
      this->header[1] = data[0];
      this->headerParsed = true;
      if (!this->checkHeader()) {
        printf("Received wrong header: %c%c\n", this->header[0],
               this->header[1]);
        this->currentState = ZDECState::ERROR;
        return;
      }
    } else {
      this->header[0] = data[0];
      this->header[1] = data[1];
      this->headerParsed = true;
      if (!this->checkHeader()) {
        printf("Received wrong header: %c%c\n", this->header[0],
               this->header[1]);
        this->currentState = ZDECState::ERROR;
        return;
      }
      this->decodePixels(data + 2, datalen - 2);
    }
  }

  void decodePixels(const uint8_t *data, std::size_t datalen) {
    for (std::size_t i = 0; i < datalen; i++) {
      this->decodeZByte(data[i]);
    }
  }

  bool checkHeader() {
    if (this->header[0] != 'Z') {
      return false;
    }
    if (Compression == ZCompressionType::Z2) {
      if (this->header[1] != '2') {
        return false;
      }
    } else if (Compression == ZCompressionType::Z3) {
      if (this->header[1] != '3') {
        return false;
      }
    }
    return true;
  }

  static uint8_t extractColor(uint8_t byte) {
    if (Compression == ZCompressionType::Z2) {
      return (byte & 0b11000000) >> 6;
    } else if (Compression == ZCompressionType::Z3) {
      return (byte & 0b11100000) >> 5;
    }
  }

  static uint8_t extractCount(uint8_t byte) {
    if (Compression == ZCompressionType::Z2) {
      return byte & 0b00111111;
    } else if (Compression == ZCompressionType::Z3) {
      return byte & 0b00011111;
    }
  }

  void decodeZByte(uint8_t byte) {
    if (this->palette.size() == 0) {
      // palette changed in meantime
      this->currentState = ZDECState::ERROR;
      return;
    }
    uint8_t color = this->extractColor(byte);
    uint8_t count = this->extractCount(byte);

    for (int j = 0; j < count; j++) {
      this->decodeBuffer[this->currentCol] =
          this->palette[color % this->palette.size()];
      this->currentCol++;

      // check if we are at the end of the row
      if (this->currentCol >= this->width) {
        this->rowCallback(this->decodeBuffer, this->currentRow);
        this->currentCol = 0;
        this->currentRow++;
        if (this->currentRow >= this->height) {
          if (j == count - 1) {
            this->currentState = ZDECState::FINISHED;
          } else {
            // more data than expected
            this->currentState = ZDECState::ERROR;
          }
          return;
        }
      }
    }
  }
};
} // namespace LaskaKit::ZivyObraz
