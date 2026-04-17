#include <cstring>

#include "zivyobrazclient.hpp"

namespace LaskaKit::ZivyObraz {

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

ZivyObrazClient::ZivyObrazClient(const char* baseUrl, DownloadCallback downloadCallback)
    : downloadCallback(downloadCallback)
{
    strncpy(m_baseUrl, baseUrl, 50);
}

void ZivyObrazClient::setApiKey(const char* apiKey)
{
    strncpy(m_apiKey, apiKey, 20);
}

int ZivyObrazClient::post(const char* path, const String& jsonPayload)
{
    strncpy(m_url, m_baseUrl, MAX_URL_LENGTH);
    strncat(m_url, path, MAX_URL_LENGTH - strlen(m_baseUrl));
    m_client.addHeader("Content-Type", "application/json");
    return this->sendRequest(m_url, "POST", jsonPayload);
}

int ZivyObrazClient::get(const char* path)
{
    strncpy(m_url, m_baseUrl, MAX_URL_LENGTH);
    strncat(m_url, path, MAX_URL_LENGTH - strlen(m_baseUrl));
    return this->sendRequest(m_url, "GET");
}

bool ZivyObrazClient::getHeader(char* buf, const char* name)
{
    if (m_client.hasHeader(name)) {
        strcpy(buf, m_client.header(name).c_str());
        return true;
    }
    return false;
}

int ZivyObrazClient::sendRequest(const char* url, const char* method, const String& payload)
        {
            Serial.println("Sending a request to server.");
            Serial.printf("Method: %s\n", method);
            Serial.printf("Url: %s\n", url);
            Serial.printf("Payload: %s\n", payload);

            m_client.begin(url);
            m_client.collectHeaders(COLLECT_HEADERS, COLLECT_HEADER_LEN);
            m_client.addHeader("X-API-key", m_apiKey);
            int httpCode = m_client.sendRequest(method, payload);

            if (httpCode != 200) {
                Serial.printf("Received error status: %d\n", httpCode);
                m_client.end();
                return httpCode;
            }

            // set content type
            ContentType contentType = ContentType::UNKNOWN;
            if (m_client.hasHeader("Content-Type")) {
                String ct = m_client.header("Content-Type");
                Serial.printf("Content-Type=%s\n", ct);
                if (ct == "application/json") {
                    contentType = ContentType::APPLICATION_JSON;
                } else if (ct == "image/z2") {
                    contentType = ContentType::IMAGE_Z2;
                } else if (ct == "image/z3") {
                    contentType = ContentType::IMAGE_Z3;
                } else if (ct == "image/bmp") {
                    contentType = ContentType::IMAGE_BMP;
                } else if (ct == "image/png") {
                    contentType = ContentType::IMAGE_PNG;
                } else if (ct == "text/plain") {
                    contentType = ContentType::TEXT_PLAIN;
                } else if (ct == "text/html") {
                    contentType = ContentType::TEXT_HTML;
                }
            }

            this->m_totalRead = 0;
            uint32_t lastData = millis();
            while (m_client.connected()) {
                size_t available = m_client.getStream().available();

                if (available) {
                    lastData = millis();
                    size_t to_process = available < BUFFER_SIZE ? available : BUFFER_SIZE;
                    size_t actuallyRead = m_client.getStream().read(m_requestBuffer, to_process);
                    if (actuallyRead > 0) {
                        this->downloadCallback(contentType, m_requestBuffer, actuallyRead);
                        m_totalRead += actuallyRead;
                    }
                } else {
                    if (millis() - lastData > 5000) {
                        log_e("Download timeout after %lu bytes", m_totalRead);
                        break;
                    }
                    yield();
                }
            }
            log_i("Download complete: %lu bytes", m_totalRead);

            m_client.end();
            return httpCode;
        }

};  // namespace LaskaKit::ZivyObraz
