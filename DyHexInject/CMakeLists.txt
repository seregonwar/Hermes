cmake_minimum_required(VERSION 3.10)
project(DyHexInject VERSION 1.0.0 LANGUAGES C)

# Imposta C11
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Evita conflitti tra winsock.h e winsock2.h
if(WIN32)
    add_definitions(-DWIN32_LEAN_AND_MEAN)
    add_definitions(-DDYHEXINJECT_EXPORTS)
endif()

# File sorgenti
set(SOURCES
    DyHexInject.c
)

# File header
set(HEADERS
    include/DyHexInject.h
    include/DyHexInjectTypes.h
)

# Crea la libreria dinamica (DLL)
add_library(DyHexInject SHARED ${SOURCES} ${HEADERS})

# Directory degli include
target_include_directories(DyHexInject
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

# Collega le librerie
target_link_libraries(DyHexInject
    PRIVATE
        psapi
)

# Imposta directory di output e suffisso per debug
set_target_properties(DyHexInject PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/bin"
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/lib"
    DEBUG_POSTFIX "d"
)

# Installa la libreria
install(TARGETS DyHexInject
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION lib
)

# Installa gli header
install(DIRECTORY include/
    DESTINATION include
    FILES_MATCHING PATTERN "*.h"
) 