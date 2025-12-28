# zephyr_library_include_directories(${CMAKE_CURRENT_SOURCE_DIR})
# zephyr_include_directories(.)
zephyr_library()
zephyr_include_directories(${CMAKE_CURRENT_SOURCE_DIR})
zephyr_library_sources(
    fit.c           # Core FIT logic
    fit_crc.c       # Checksum calculation
    fit_profile.c   # The minimal profile you just created
    fit_convert.c   # Helpers to encode/decode messages
    fit_product.c   # Helpers for timestamps and types
    fit_ram.c       # RAM buffer helpers (Useful for embedded)
)
