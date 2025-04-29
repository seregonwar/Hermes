cmake_minimum_required(VERSION 3.15)
project(DyForge VERSION 1.0.0 LANGUAGES CXX)

# Imposta C++17
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Definizioni per Windows
if(WIN32)
    add_definitions(-DWIN32_LEAN_AND_MEAN)
endif()

# File sorgenti
set(SOURCES
    main.cpp
)

# Crea l'eseguibile
add_executable(DyForge ${SOURCES})

# Directory degli include
target_include_directories(DyForge
    PRIVATE
        ${CMAKE_SOURCE_DIR}/DyMain/include
        ${CMAKE_SOURCE_DIR}/DyHexInject/include
)

# Imposta la directory di output
set_target_properties(DyForge PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin/Debug
)

# Collega le librerie
target_link_libraries(DyForge
    PRIVATE
        ${CMAKE_BINARY_DIR}/DyMain/Debug/DyMain.lib
        ${CMAKE_BINARY_DIR}/DyHexInject/Debug/DyHexInjectd.lib
)

# Copia le DLL necessarie nella directory di output
add_custom_command(TARGET DyForge POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${CMAKE_BINARY_DIR}/bin/Debug/DyMain.dll
        ${CMAKE_BINARY_DIR}/bin/Debug/DyHexInjectd.dll
        ${CMAKE_BINARY_DIR}/bin/Debug
) 