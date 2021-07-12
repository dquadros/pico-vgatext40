cmake_minimum_required(VERSION 3.13)


# Pull in PICO SDK (must be before project)
include(pico_sdk_import.cmake)

# We also need PICO EXTRAS
include(pico_extras_import.cmake)

# Initialize the Pico SDK
pico_sdk_init()


project(textvideo_project)

add_executable(textvideo
    textvideo.c
)

target_compile_definitions(textvideo PRIVATE
        PICO_SCANVIDEO_SCANLINE_BUFFER_COUNT=20
        PICO_SCANVIDEO_MAX_SCANLINE_BUFFER_WORDS=360
)

target_link_libraries(textvideo PRIVATE
        pico_multicore
        pico_scanvideo_dpi
        pico_stdlib
        )

pico_add_extra_outputs(textvideo)


