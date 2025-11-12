# CO2 Effect Extension for Axmol

set(LIB_NAME co2effect)

set(${LIB_NAME}_HEADERS
    include/C3FileReader.h
    include/EffectConfig.h
    include/CO2EffectPlayer.h
)

set(${LIB_NAME}_SOURCES
    src/C3FileReader.cpp
    src/EffectConfig.cpp
    src/CO2EffectPlayer.cpp
)

add_library(${LIB_NAME} STATIC
    ${${LIB_NAME}_HEADERS}
    ${${LIB_NAME}_SOURCES}
)

target_include_directories(${LIB_NAME} PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(${LIB_NAME} PUBLIC
    ${_AX_CORE_LIB}
)

set_target_properties(${LIB_NAME} PROPERTIES
    FOLDER "extensions"
)
