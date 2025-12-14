# 1. Create standard Zephyr library (Merged into final binary automatically)
zephyr_library()

# 2. Add Sources
zephyr_library_sources(
    src/RowingEngine.cpp
    src/MovingFlankDetector.cpp
    src/MovingAverager.cpp
)

# 3. Expose headers (for internal use)
zephyr_library_include_directories(include)

# 4. Link dependencies (headers from core)
# We use zephyr_library_link_libraries so this module can find RowingData.h
# zephyr_library_link_libraries(rowing_core)
