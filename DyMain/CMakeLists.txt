cmake_minimum_required(VERSION 3.10)

# Nome del progetto
project(DyMain)

# Imposta il C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Opzioni di compilazione
option(BUILD_SHARED_LIBS "Build shared libraries" ON)

# Directory di output
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# Directory delle librerie
set(LIB_DIR "${CMAKE_CURRENT_SOURCE_DIR}/lib")
set(INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/include")

# Configura vcpkg
if(WIN32)
    if(DEFINED ENV{VCPKG_ROOT})
        set(CMAKE_TOOLCHAIN_FILE "$ENV{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake" CACHE STRING "")
        set(CMAKE_PREFIX_PATH "${CMAKE_PREFIX_PATH};$ENV{VCPKG_ROOT}/installed/x64-windows")
    else()
        message(WARNING "VCPKG_ROOT non impostato. Alcune librerie potrebbero non essere trovate.")
    endif()
endif()

# Trova la libreria Capstone
find_library(CAPSTONE_LIBRARY
    NAMES capstone
    PATHS
        ${LIB_DIR}
        "${CMAKE_CURRENT_SOURCE_DIR}/lib"
    NO_DEFAULT_PATH
)

find_path(CAPSTONE_INCLUDE_DIR
    NAMES capstone/capstone.h
    PATHS
        ${INCLUDE_DIR}
        "${CMAKE_CURRENT_SOURCE_DIR}/include"
    NO_DEFAULT_PATH
)

if(NOT CAPSTONE_LIBRARY OR NOT CAPSTONE_INCLUDE_DIR)
    message(FATAL_ERROR "Capstone non trovato. Verifica che capstone.lib sia presente in ${LIB_DIR} e capstone.h in ${INCLUDE_DIR}")
endif()

# Trova Microsoft Detours
find_library(DETOURS_LIBRARY
    NAMES detours
    PATHS
        ${LIB_DIR}
        "${CMAKE_CURRENT_SOURCE_DIR}/lib"
    NO_DEFAULT_PATH
)

find_path(DETOURS_INCLUDE_DIR
    NAMES detours.h
    PATHS
        ${INCLUDE_DIR}
        "${CMAKE_CURRENT_SOURCE_DIR}/include"
    NO_DEFAULT_PATH
)

if(NOT DETOURS_LIBRARY OR NOT DETOURS_INCLUDE_DIR)
    message(FATAL_ERROR "Microsoft Detours non trovato. Verifica che detours.lib sia presente in ${LIB_DIR} e detours.h in ${INCLUDE_DIR}")
endif()

# Aggiungi i file sorgente
set(SOURCES
    dll_main.cpp
    mongoose.c
)

# Aggiungi le directory di include
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CAPSTONE_INCLUDE_DIR}
    ${DETOURS_INCLUDE_DIR}
)

# Crea la libreria condivisa
add_library(${PROJECT_NAME} SHARED ${SOURCES})

# Definisci il simbolo di esportazione
target_compile_definitions(${PROJECT_NAME} PRIVATE 
    "BUILDING_DLL"
    "CAPSTONE_USE_SYS_DYN_MEM"
)

# Collega le librerie necessarie
target_link_libraries(${PROJECT_NAME} PRIVATE
    ws2_32
    ${CAPSTONE_LIBRARY}
    ${DETOURS_LIBRARY}
)

# Installa la libreria
install(TARGETS ${PROJECT_NAME}
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
)

# Installa gli header
install(DIRECTORY include/
    DESTINATION include
    FILES_MATCHING PATTERN "*.h"
) 