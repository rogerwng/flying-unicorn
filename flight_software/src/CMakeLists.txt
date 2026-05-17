message(STATUS "Building lib_fsw")

add_library(lib_fsw STATIC)

file(GLOB BOARD_CONFIG CONFIGURE_DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/board_config/*.cpp
)

file(GLOB SENSOR_SOURCES CONFIGURE_DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/sensors/*.cpp
)

file(GLOB UTIL_SOURCES CONFIGURE_DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/utils/*.cpp
)

target_sources(lib_fsw PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/main.cpp
    ${BOARD_CONFIG}
    ${SENSOR_SOURCES}
    ${UTIL_SOURCES}
)

target_include_directories(lib_fsw PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/board_config
    ${CMAKE_CURRENT_SOURCE_DIR}/sensors
    ${CMAKE_CURRENT_SOURCE_DIR}/utils
)

target_link_libraries(lib_fsw PUBLIC
    stm32cubemx
)

if (CMAKE_BUILD_TYPE STREQUAL "Debug")
    message(STATUS "Enabling debug logs")
    target_compile_definitions(lib_fsw PRIVATE ENABLE_LOGGING)
endif()

# Link user code into the main CubeMX/BSP executable.
target_link_libraries(${CMAKE_PROJECT_NAME}
    lib_fsw
)