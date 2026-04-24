#include <cstring>

#include "zivyobrazclient.hpp"


namespace {
    // interesting headers to collect
    constexpr size_t COLLECT_HEADER_LEN = 8;
    const char* const COLLECT_HEADERS[COLLECT_HEADER_LEN] = {
        "Content-Type",
        "Content-Length",
        "Data-Length",
        "Timestamp",
        "Sleep",
        "SleepSeconds",
        "PreciseSleep",
        "Rotate",
    };

    const struct {
        const char*  mime;
        LaskaKit::ZivyObraz::ContentType  type;
    } ctLookup[] = {
        { "application/json", LaskaKit::ZivyObraz::ContentType::APPLICATION_JSON },
        { "image/z2",         LaskaKit::ZivyObraz::ContentType::IMAGE_Z2         },
        { "image/z3",         LaskaKit::ZivyObraz::ContentType::IMAGE_Z3         },
        { "image/bmp",        LaskaKit::ZivyObraz::ContentType::IMAGE_BMP        },
        { "image/png",        LaskaKit::ZivyObraz::ContentType::IMAGE_PNG        },
        { "text/plain",       LaskaKit::ZivyObraz::ContentType::TEXT_PLAIN       },
        { "text/html",        LaskaKit::ZivyObraz::ContentType::TEXT_HTML        },
    };


    // based on https://github.com/plageoj/urlencode
    // todo -> this is dangerous as it does not check bounds
    //         user is responsible for providing large enough buffer (encodedMsg)
    void urlEncode(const char *msg, char* encodedMsg) {
        const char *hex = "0123456789ABCDEF";
        size_t i = 0;

        while (*msg != '\0') {
        if (
            ('a' <= *msg && *msg <= 'z') || ('A' <= *msg && *msg <= 'Z') || ('0' <= *msg && *msg <= '9') || *msg == '-' || *msg == '_' || *msg == '.' || *msg == '~' || *msg == ':') {
            encodedMsg[i++] = *msg;
        } else {
            encodedMsg[i++] = '%';
            encodedMsg[i++] = hex[(unsigned char)*msg >> 4];
            encodedMsg[i++] = hex[*msg & 0xf];
        }
        msg++;
        }
    }
}

namespace LaskaKit::ZivyObraz {

ZivyObrazClient::ZivyObrazClient(const char* baseUrl)
{
    strncpy(m_baseUrl, baseUrl, sizeof(m_baseUrl));
    m_baseUrl[sizeof(m_baseUrl) - 1] = '\0';
}

void ZivyObrazClient::setApiKey(const char* apiKey)
{
    strncpy(m_apiKey, apiKey, sizeof(m_apiKey));
    m_apiKey[sizeof(m_apiKey) - 1] = '\0';
}

int ZivyObrazClient::post(const char* path, const String& jsonPayload)
{
    strncpy(m_url, m_baseUrl, MAX_URL_LENGTH);
    strncat(m_url, path, MAX_URL_LENGTH - strlen(m_baseUrl) - 1);
    m_client.addHeader("Content-Type", "application/json");
    return this->sendRequest(m_url, "POST", jsonPayload);
}

int ZivyObrazClient::get(const char* path)
{
    strncpy(m_url, m_baseUrl, MAX_URL_LENGTH);
    strncat(m_url, path, MAX_URL_LENGTH - strlen(m_baseUrl) - 1);
    return this->sendRequest(m_url, "GET");
}

bool ZivyObrazClient::getHeader(char* buf, size_t buflen, const char* name)
{
    if (m_client.hasHeader(name)) {
        strncpy(buf, m_client.header(name).c_str(), buflen);
        buf[buflen - 1] = '\0';
        return true;
    }
    return false;
}

ContentHandler ZivyObrazClient::selectHandler()
{
    if (!m_client.hasHeader("Content-Type")) {
        log_e("Response does not contain 'Content-Type' header");
        return nullptr;
    }

    const String ct = m_client.header("Content-Type");
    for (const auto& entry : ctLookup) {
        if (ct.startsWith(entry.mime)) {
            return m_handlers[static_cast<size_t>(entry.type)];
        }
    }

    return m_handlers[static_cast<size_t>(ContentType::UNKNOWN)];
}

int ZivyObrazClient::readStream()
{
    if (!m_active) { return -1; }

    ContentHandler handler = this->selectHandler();
    if (!handler) {
        return -1;
    }

    int totalRead = 0;
    uint32_t lastData = millis();
    while (m_client.connected()) {
        size_t available = m_client.getStream().available();

        if (available) {
            lastData = millis();
            size_t to_process = available < BUFFER_SIZE ? available : BUFFER_SIZE;
            size_t actuallyRead = m_client.getStream().read(m_requestBuffer, to_process);
            if (actuallyRead > 0) {
                if (!handler(m_requestBuffer, actuallyRead)) {
                    log_e("Handler signalled failure after %d bytes", totalRead);
                    break;
                }
                totalRead += actuallyRead;
            }
        } else {
            if (millis() - lastData > 5000) {
                log_e("Download timeout after %d bytes", totalRead);
                break;
            }
            yield();
        }
    }
    m_client.end();
    m_active = false;
    return totalRead;
}


int ZivyObrazClient::sendRequest(const char* url, const char* method, const String& payload)
{
    m_client.begin(url);
    m_active = true;
    m_client.collectHeaders(COLLECT_HEADERS, COLLECT_HEADER_LEN);
    m_client.addHeader("X-API-key", m_apiKey);
    int httpCode = m_client.sendRequest(method, payload);
    if (httpCode != 200) {
        m_client.end();
        m_active = false;
    }
    return httpCode;
}

};  // namespace LaskaKit::ZivyObraz
