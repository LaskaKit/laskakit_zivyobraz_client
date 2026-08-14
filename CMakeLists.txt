idf_component_register(
    SRCS
        "src/zivyobrazclient.cpp"
        "src/zdecoder.c"
        "src/bmpdecoder.c"
    INCLUDE_DIRS "src"
    REQUIRES esp_http_client
)
