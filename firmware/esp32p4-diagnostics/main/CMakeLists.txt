idf_component_register(
    SRCS "main.c"
    INCLUDE_DIRS "."
    PRIV_REQUIRES efuse esp_driver_sdmmc esp_timer fatfs sdmmc
)
