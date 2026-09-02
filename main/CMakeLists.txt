idf_component_register(
    SRCS "main.c"
         "pet_app.c"
         "pet_state.c"
         "font_pet_zh_14.c"
         "ui_pixel.c"
         "ui_pixel_math.c"
    INCLUDE_DIRS "."
    REQUIRES bsp esp_app_format esp_hw_support esp_rom esp_timer nvs_flash
    EMBED_FILES
        "assets/compressed/idle.zframes"
        "assets/compressed/happy.zframes"
        "assets/compressed/sad.zframes"
        "assets/compressed/surprised.zframes"
        "assets/compressed/play.zframes"
        "assets/compressed/rest.zframes"
        "assets/compressed/cool.zframes"
)
