message(STATUS "Building lib_fsw")

add_library(lib_fsw STATIC)

file(GLOB INIT_SOURCES CONFIGURE_DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/init/*.cpp
)

file(GLOB SENSOR_SOURCES CONFIGURE_DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/sensors/*.cpp
)

file(GLOB UTIL_SOURCES CONFIGURE_DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/utils/*.cpp
)

target_sources(lib_fsw PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/main.cpp
    ${INIT_SOURCES}
    ${SENSOR_SOURCES}
    ${UTIL_SOURCES}
)

target_include_directories(lib_fsw PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/init
    ${CMAKE_CURRENT_SOURCE_DIR}/sensors
    ${CMAKE_CURRENT_SOURCE_DIR}/utils
)

target_link_libraries(lib_fsw PUBLIC
    stm32cubemx
)

# Link user code into the main CubeMX/BSP executable.
target_link_libraries(${CMAKE_PROJECT_NAME}
    lib_fsw
)