#pragma once

#include <cstdint>
#include <cstring>

namespace LaskaKit::ZivyObraz {
    constexpr size_t MAX_PARAMS = 15;
    constexpr size_t MAX_VALUE_LEN = 20;
    constexpr size_t MAX_KEY_LEN = 20;
    constexpr size_t MAX_URL_LENGTH = 256;

    struct HttpParam
    {
        char key[MAX_KEY_LEN] = "";
        char value[MAX_VALUE_LEN] = "";
    };

    struct HttpParams
    {
      HttpParam params[MAX_PARAMS];
      size_t count = 0;

      bool add(const char* key, const char* value);
      size_t queryStrLength() const;
      void buildQuery(char* buffer, size_t buflen) const;
    };

    class ZivyobrazClient
    {
    protected:
        HttpParams m_httpParams;
        char m_url[MAX_URL_LENGTH];
    public:
        ZivyobrazClient()
        {}
        virtual ~ZivyobrazClient() = default;
        virtual void addParam(const char* key, const char* value)
        {
            this->m_httpParams.add(key, value);
        }
        virtual void get() = 0;
        virtual void getHeader(char* buf, const char* name) = 0;
    };
}
