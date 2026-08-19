cmake_minimum_required(VERSION 3.15)
project(Lofty CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Gather all C++ source files from src/
file(GLOB SOURCES "src/*.cpp")

add_executable(Lofty ${SOURCES})

target_include_directories(Lofty PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    # Profile Guided Optimization (PGO) Support
    # Usage: 
    # 1. Generate profile: cmake -DENABLE_PGO=GENERATE .. && make && ./lofty bench
    # 2. Use profile: cmake -DENABLE_PGO=USE .. && make
    if(ENABLE_PGO STREQUAL "GENERATE")
        set(PGO_FLAGS "-fprofile-generate")
        set(PGO_LINK_FLAGS "-fprofile-generate")
    elseif(ENABLE_PGO STREQUAL "USE")
        set(PGO_FLAGS "-fprofile-use -fprofile-correction")
        set(PGO_LINK_FLAGS "-fprofile-use")
    else()
        set(PGO_FLAGS "")
        set(PGO_LINK_FLAGS "")
    endif()

    target_compile_options(Lofty PRIVATE 
        -O3 
        -march=native 
        -mbmi2          # Enables PEXT instruction
        -mpopcnt        # Enables POPCNT instruction
        -mavx2          # Enables AVX2 for SIMD operations
        -funroll-loops 
        -fno-exceptions 
        -fno-rtti
        -ffast-math     # Aggressive math optimizations
        -fno-math-errno # Skip errno checks for math functions
        ${PGO_FLAGS}
        -Wall 
        -Wextra
    )
    # Static linking ensures no missing DLLs in GUIs!
    target_link_options(Lofty PRIVATE -static -static-libgcc -static-libstdc++ -lpthread ${PGO_LINK_FLAGS})
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