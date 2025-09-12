#include <cstring>

#include "zivyobrazclient.hpp"

using namespace LaskaKit::ZivyObraz;


bool HttpParams::add(const char* key, const char* value)
{
    if (this->count >= MAX_PARAMS) {
        return false;
    }

    strncpy(params[this->count].key, key, MAX_KEY_LEN-1);
    params[this->count].key[MAX_KEY_LEN - 1] = '\0';

    strncpy(params[this->count].value, value, MAX_VALUE_LEN-1);
    params[this->count].value[MAX_VALUE_LEN - 1] = '\0';

    this->count++;
    return true;
}

size_t HttpParams::queryStrLength() const
{
    size_t length = 2 * this->count;  // '&' + '=' + '\0'
    for (size_t i = 0; i < this->count; i++) {
        length += strlen(this->params[i].key);
        length += strlen(this->params[i].value);
    }
    return length;
}

void HttpParams::buildQuery(char* buffer, size_t buflen) const
{
    for (size_t i = 0; i < this->count; i++) {
        strncat(buffer, this->params[i].key, buflen - strlen(buffer) - 1);
        strncat(buffer, "=", buflen - strlen(buffer) - 1);
        strncat(buffer, this->params[i].value, buflen - strlen(buffer) - 1);
        if (i != count - 1) {
            strncat(buffer, "&", buflen - strlen(buffer) - 1);
        }
    }
}
