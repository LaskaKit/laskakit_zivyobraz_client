#pragma once

#include <cstddef>
#include <cstdint>

#include "esp_http_client.h"

namespace LaskaKit::ZivyObraz {
    constexpr size_t MAX_URL_LENGTH = 256;
    constexpr size_t BUFFER_SIZE = 8192;  // 2^13
    /// Maximum length of the base URL string (including null terminator).
    constexpr size_t MAX_BASE_URL_LENGTH = 51;
    /// Maximum length of the API key string (including null terminator).
    constexpr size_t MAX_API_KEY_LENGTH = 21;
    /// Maximum length of a collected response header value (including null terminator).
    constexpr size_t MAX_HEADER_VALUE_LENGTH = 64;

    enum class ContentType
    {
        APPLICATION_JSON,
        IMAGE_Z2,
        IMAGE_Z3,
        IMAGE_BMP,
        IMAGE_PNG,
        TEXT_PLAIN,
        TEXT_HTML,
        APPLICATION_OCTET_STREAM,
        UNKNOWN,
        COUNT = 9,
    };

    enum class ColorType
    {
        BW,
        G4,
        G8,
        BWR,
        BWY,
        BWRY,
        C4,
        C7,
    };


    /**
     * @brief Callback invoked for each chunk of data read from the response stream.
     *
     * @param downloadedData  Pointer to the buffer containing the chunk.
     * @param datalen         Number of valid bytes in the buffer.
     * @return                @c true if the chunk was processed successfully;
     *                        @c false to signal an error and abort the download.
     */
    typedef bool (*ContentHandler)(const uint8_t* downloadedData, size_t datalen);

    class ZivyObrazClient {
    public:
        /// Number of response headers collected per request; see getHeader().
        static constexpr size_t COLLECT_HEADER_LEN = 8;

    private:
        esp_http_client_handle_t m_client = nullptr;
        bool m_active = false;
        uint8_t m_requestBuffer[BUFFER_SIZE];

        ContentHandler m_handlers[static_cast<size_t>(ContentType::COUNT)] = {};

        char m_baseUrl[MAX_BASE_URL_LENGTH] = {};
        char m_url[MAX_URL_LENGTH] = {};
        char m_apiKey[MAX_API_KEY_LENGTH] = {};

        char m_headerValues[COLLECT_HEADER_LEN][MAX_HEADER_VALUE_LENGTH] = {};
        bool m_headerPresent[COLLECT_HEADER_LEN] = {};

    public:

        ZivyObrazClient() = default;

        /// Convenience constructor; equivalent to ZivyObrazClient() followed by setBaseUrl(baseUrl).
        explicit ZivyObrazClient(const char* baseUrl) { setBaseUrl(baseUrl); }

        ~ZivyObrazClient();

        // Owns a live esp_http_client_handle_t; not safe to copy.
        ZivyObrazClient(const ZivyObrazClient&) = delete;
        ZivyObrazClient& operator=(const ZivyObrazClient&) = delete;

        /**
         * @brief Sets a server base URL.
         *
         * @param baseUrl  Null-terminated base URL of the ZivyObraz server.
         *                 Truncated silently if longer than
         *                 MAX_BASE_URL_LENGTH - 1 characters.
         */
        void setBaseUrl(const char* baseUrl);

        /**
         * @brief Registers a content handler for a specific media type.
         *
         * When readStream() is called after a successful response, the handler
         * whose ContentType matches the @c Content-Type response header is
         * invoked once per read buffer.
         *
         * @param ct  The content type this handler should be bound to.
         * @param h   Function pointer to the handler, or @c nullptr to remove
         *            a previously registered handler.
         */
        void registerHandler(ContentType ct, ContentHandler h) {
            m_handlers[static_cast<size_t>(ct)] = h;
        }

        /**
         * @brief Sets the API key sent in the @c X-API-Key header of every request.
         *
         * @param apiKey  Null-terminated key string.  Truncated silently if
         *                longer than MAX_API_KEY_LENGTH - 1 characters.
         */
        void setApiKey(const char* apiKey);

        /**
         * @brief Sends an HTTP POST request with a JSON body.
         *
         * The @c Content-Type: application/json header is added automatically.
         *
         * @param path         Request path appended to the base URL
         *                     (e.g. @c "/api/v1/update").
         * @param jsonPayload  Null-terminated JSON-encoded request body.
         * @return             HTTP status code, or -1 on a transport-level error.
         */
        int post(const char* path, const char* jsonPayload);

        /**
         * @brief Sends an HTTP GET request.
         *
         * @param path  Request path appended to the base URL.
         * @return      HTTP status code, or -1 on a transport-level error.
         */
        int get(const char* path);

        /**
         * @brief Reads the response body in chunks and dispatches them to the
         *        appropriate ContentHandler.
         *
         * Must be called after a successful post() or get() (HTTP 200).
         * Blocks until the connection closes or a read times out.
         *
         * @return Total number of bytes read, or -1 if no handler was found
         *         for the response Content-Type.
         */
        int readStream();

        /**
         * @brief Retrieves a collected response header by name.
         *
         * Only headers listed in COLLECT_HEADERS are available.
         *
         * @param buf    Caller-allocated destination buffer.
         * @param buflen The size of the caller-allocated buffer.
         * @param name   Null-terminated header name (case-insensitive).
         * @return       @c true if the header was present and copied into @p buf;
         *               @c false otherwise (@p buf is left unchanged).
         */
        bool getHeader(char* buf, size_t buflen, const char* name) const;

    private:
        int sendRequest(const char* url, esp_http_client_method_t method,
                         const char* payload, size_t payloadLen);
        ContentHandler selectHandler();
        void resetHeaders();
        void storeHeader(const char* key, const char* value);
        static esp_err_t httpEventHandler(esp_http_client_event_t* evt);
    };
}
