idf_component_register(SRCS "medsenger_http_requests.c"
                    INCLUDE_DIRS "include"
                    PRIV_REQUIRES esp_http_client esp-tls pilld_common
                    EMBED_TXTFILES howsmyssl_com_root_cert.pem)
