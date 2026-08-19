cmake_minimum_required(VERSION 3.15)
project(Lofty CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Gather all C++ source files from src/
file(GLOB SOURCES "src/*.cpp")

add_executable(Lofty ${SOURCES})

target_include_directories(Lofty PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(Lofty PRIVATE 
        -O3 
        -march=native 
        -mbmi2          # Enables PEXT instruction
        -mpopcnt        # Enables POPCNT instruction
        -mavx2          # Enables AVX2 for SIMD operations
        -funroll-loops 
        -fno-exceptions 
        -fno-rtti
        -Wall 
        -Wextra
    )
    # Static linking ensures no missing DLLs in GUIs!
    target_link_options(Lofty PRIVATE -static -static-libgcc -static-libstdc++ -lpthread)
endif()

if(MSVC)
    target_compile_options(Lofty PRIVATE 
        /O2             # Max speed
        /arch:AVX2      # Implies BMI2 and POPCNT on x86-64
        /fp:fast 
        /EHsc           # Standard C++ exception handling
        /W3
    )
endif()