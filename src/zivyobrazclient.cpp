#include <cstring>
#include <strings.h>

#include "esp_log.h"

#include "zivyobrazclient.hpp"

using namespace LaskaKit::ZivyObraz;

namespace {
    constexpr const char* TAG = "ZivyObrazClient";

    // interesting headers to collect
    const char* const COLLECT_HEADERS[ZivyObrazClient::COLLECT_HEADER_LEN] = {
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
    } contentTypeLookup[] = {
        { "application/json",         ContentType::APPLICATION_JSON         },
        { "image/z2",                 ContentType::IMAGE_Z2                 },
        { "image/z3",                 ContentType::IMAGE_Z3                 },
        { "image/bmp",                ContentType::IMAGE_BMP                },
        { "image/png",                ContentType::IMAGE_PNG                },
        { "text/plain",               ContentType::TEXT_PLAIN               },
        { "text/html",                ContentType::TEXT_HTML                },
        { "application/octet-stream", ContentType::APPLICATION_OCTET_STREAM },
    };


    // What to declare in the request
    [[maybe_unused]] const struct {
        const char* name;
        ColorType type;
    } colorTypeLookup[] = {
        {"BW", ColorType::BW},
        {"GRAYSCALE", ColorType::G4},
        {"8G", ColorType::G8},
        {"RBW", ColorType::BWR},
        {"YBW", ColorType::BWY},
        {"4C", ColorType::BWRY},
        {"7C", ColorType::C7},
    };


    // based on https://github.com/plageoj/urlencode
    // todo -> this is dangerous as it does not check bounds
    //         user is responsible for providing large enough buffer (encodedMsg)
    [[maybe_unused]] void urlEncode(const char *msg, char* encodedMsg) {
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


ZivyObrazClient::~ZivyObrazClient()
{
    if (m_client) {
        esp_http_client_cleanup(m_client);
        m_client = nullptr;
    }
}

void ZivyObrazClient::setBaseUrl(const char* baseUrl)
{
    strncpy(m_baseUrl, baseUrl, sizeof(m_baseUrl));
    m_baseUrl[sizeof(m_baseUrl) - 1] = '\0';
}

void ZivyObrazClient::setApiKey(const char* apiKey)
{
    strncpy(m_apiKey, apiKey, sizeof(m_apiKey));
    m_apiKey[sizeof(m_apiKey) - 1] = '\0';
}

int ZivyObrazClient::post(const char* path, const char* jsonPayload)
{
    strncpy(m_url, m_baseUrl, MAX_URL_LENGTH);
    m_url[MAX_URL_LENGTH - 1] = '\0';
    strncat(m_url, path, MAX_URL_LENGTH - strlen(m_url) - 1);
    size_t payloadLen = jsonPayload ? strlen(jsonPayload) : 0;
    return this->sendRequest(m_url, HTTP_METHOD_POST, jsonPayload, payloadLen);
}

int ZivyObrazClient::get(const char* path)
{
    strncpy(m_url, m_baseUrl, MAX_URL_LENGTH);
    m_url[MAX_URL_LENGTH - 1] = '\0';
    strncat(m_url, path, MAX_URL_LENGTH - strlen(m_url) - 1);
    return this->sendRequest(m_url, HTTP_METHOD_GET, nullptr, 0);
}

void ZivyObrazClient::resetHeaders()
{
    memset(m_headerPresent, 0, sizeof(m_headerPresent));
}

void ZivyObrazClient::storeHeader(const char* key, const char* value)
{
    for (size_t i = 0; i < COLLECT_HEADER_LEN; i++) {
        if (strcasecmp(COLLECT_HEADERS[i], key) == 0) {
            strncpy(m_headerValues[i], value, MAX_HEADER_VALUE_LENGTH);
            m_headerValues[i][MAX_HEADER_VALUE_LENGTH - 1] = '\0';
            m_headerPresent[i] = true;
            break;
        }
    }
}

esp_err_t ZivyObrazClient::httpEventHandler(esp_http_client_event_t* evt)
{
    if (evt->event_id == HTTP_EVENT_ON_HEADER) {
        auto* self = static_cast<ZivyObrazClient*>(evt->user_data);
        self->storeHeader(evt->header_key, evt->header_value);
    }
    return ESP_OK;
}

bool ZivyObrazClient::getHeader(char* buf, size_t buflen, const char* name) const
{
    for (size_t i = 0; i < COLLECT_HEADER_LEN; i++) {
        if (m_headerPresent[i] && strcasecmp(COLLECT_HEADERS[i], name) == 0) {
            strncpy(buf, m_headerValues[i], buflen);
            buf[buflen - 1] = '\0';
            return true;
        }
    }
    return false;
}

ContentHandler ZivyObrazClient::selectHandler()
{
    char contentType[MAX_HEADER_VALUE_LENGTH];
    if (!getHeader(contentType, sizeof(contentType), "Content-Type")) {
        ESP_LOGE(TAG, "Response does not contain 'Content-Type' header");
        return nullptr;
    }

    ESP_LOGI(TAG, "Content-Type=%s", contentType);

    for (const auto& entry : contentTypeLookup) {
        if (strncmp(contentType, entry.mime, strlen(entry.mime)) == 0) {
            return m_handlers[static_cast<size_t>(entry.type)];
        }
    }

    return m_handlers[static_cast<size_t>(ContentType::UNKNOWN)];
}

int ZivyObrazClient::readStream()
{
    ESP_LOGV(TAG, "readStream - m_active=%d", m_active);
    if (!m_active || !m_client) { return -1; }

    ContentHandler handler = selectHandler();
    if (!handler) {
        ESP_LOGE(TAG, "Handler not set.");
        esp_http_client_close(m_client);
        m_active = false;
        return -1;
    }

    int totalRead = 0;
    int64_t contentLength = esp_http_client_get_content_length(m_client);
    while (true) {
        int readLen = esp_http_client_read(m_client, reinterpret_cast<char*>(m_requestBuffer), BUFFER_SIZE);
        if (readLen < 0) {
            ESP_LOGE(TAG, "Read error after %d bytes", totalRead);
            break;
        }
        if (readLen == 0) {
            if (esp_http_client_is_complete_data_received(m_client)) {
                ESP_LOGI(TAG, "Download complete: %d bytes", totalRead);
            } else {
                ESP_LOGW(TAG, "Connection closed early after %d bytes", totalRead);
            }
            break;
        }

        if (!handler(m_requestBuffer, readLen)) {
            ESP_LOGE(TAG, "Handler signalled failure after %d bytes", totalRead);
            break;
        }
        totalRead += readLen;

        if (contentLength > 0 && totalRead >= contentLength) {
            ESP_LOGI(TAG, "Download complete: %d bytes", totalRead);
            break;
        }
    }

    esp_http_client_close(m_client);
    m_active = false;
    return totalRead;
}


int ZivyObrazClient::sendRequest(const char* url, esp_http_client_method_t method,
                                  const char* payload, size_t payloadLen)
{
    ESP_LOGI(TAG, "%s %s", method == HTTP_METHOD_POST ? "POST" : "GET", url);
    if (payload) {
        ESP_LOGD(TAG, "%s", payload);
    }

    resetHeaders();

    if (m_client) {
        esp_http_client_cleanup(m_client);
        m_client = nullptr;
    }

    esp_http_client_config_t config = {};
    config.url = url;
    config.method = method;
    config.event_handler = httpEventHandler;
    config.user_data = this;
    config.disable_auto_redirect = false;

    m_client = esp_http_client_init(&config);
    if (!m_client) {
        ESP_LOGE(TAG, "Failed to initialise HTTP client");
        return -1;
    }

    esp_http_client_set_header(m_client, "X-API-key", m_apiKey);
    if (payload) {
        esp_http_client_set_header(m_client, "Content-Type", "application/json");
    }

    esp_err_t err = esp_http_client_open(m_client, static_cast<int>(payloadLen));
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Connection failed: %s", esp_err_to_name(err));
        esp_http_client_cleanup(m_client);
        m_client = nullptr;
        return -1;
    }

    if (payload && payloadLen > 0) {
        int written = esp_http_client_write(m_client, payload, static_cast<int>(payloadLen));
        if (written < 0 || static_cast<size_t>(written) != payloadLen) {
            ESP_LOGE(TAG, "Failed to write request payload");
            esp_http_client_close(m_client);
            esp_http_client_cleanup(m_client);
            m_client = nullptr;
            return -1;
        }
    }

    int64_t contentLength = esp_http_client_fetch_headers(m_client);
    if (contentLength < 0) {
        ESP_LOGE(TAG, "Failed to fetch response headers");
        esp_http_client_close(m_client);
        esp_http_client_cleanup(m_client);
        m_client = nullptr;
        return -1;
    }

    int httpCode = esp_http_client_get_status_code(m_client);
    m_active = (httpCode == 200);
    if (!m_active) {
        esp_http_client_close(m_client);
        esp_http_client_cleanup(m_client);
        m_client = nullptr;
    }
    return httpCode;
}


};  // namespace LaskaKit::ZivyObraz
