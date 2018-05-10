# For more information about using CMake with Android Studio, read the
# documentation: https://d.android.com/studio/projects/add-native-code.html

# Sets the minimum version of CMake required to build the native library.

cmake_minimum_required(VERSION 3.4.1)
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -O3 -Wall -D_FILE_OFFSET_BITS=64")
set(CMAKE_BUILD_TYPE Release)

include_directories(src/main/cpp/ConnectionSTREAM/)
include_directories(src/main/cpp/ConnectionEVB7COM/)
include_directories(src/main/cpp/Connection_uLimeSDR/)
include_directories(src/main/cpp/GFIR/)
include_directories(src/main/cpp/ADF4002/)
include_directories(src/main/cpp/API/)
include_directories(src/main/cpp/FPGA_common/)
include_directories(src/main/cpp/Si5351C/)
include_directories(src/main/cpp/protocols/)
include_directories(src/main/cpp/lms7002m_mcu/)
include_directories(src/main/cpp/cpp-feather-ini-parser/)
include_directories(src/main/cpp/sqlite-3.12/)
include_directories(src/main/cpp/lms7002m/)
include_directories(src/main/cpp/ConnectionRegistry/)
include_directories(src/main/cpp/inc/)
include_directories(src/main/cpp/libusb/)
include_directories(src/main/cpp/libusb/os/)
include_directories(src/main/cpp/lime/)
include_directories(src/main/cpp/)

file(GLOB lime_suite_lib
        src/main/cpp/ConnectionSTREAM/*.cpp
        src/main/cpp/ConnectionEVB7COM/*.cpp
        src/main/cpp/Connection_uLimeSDR/*.cpp
        src/main/cpp/GFIR/*.c
        src/main/cpp/ADF4002/*.cpp
        src/main/cpp/API/*.cpp
        src/main/cpp/FPGA_common/*.cpp
        src/main/cpp/Si5351C/*.cpp
        src/main/cpp/protocols/*.cpp
        src/main/cpp/lms7002m_mcu/*.cpp
        src/main/cpp/cpp-feather-ini-parser/*.cpp
        src/main/cpp/sqlite-3.12/*.c
        src/main/cpp/lms7002m/*.cpp
        src/main/cpp/ConnectionRegistry/*.cpp
        src/main/cpp/inc/*.cpp
        src/main/cpp/libusb/*.c
        src/main/cpp/libusb/os/*.c
        src/main/cpp/lime/*.cpp
        src/main/cpp/*.c
)

# Creates and names a library, sets it as either STATIC
# or SHARED, and provides the relative paths to its source code.
# You can define multiple libraries, and CMake builds them for you.
# Gradle automatically packages shared libraries with your APK.

add_library( # Sets the name of the library.
             native-lib

             # Sets the library as a shared library.
             SHARED
             ${lime_suite_lib}

             # Provides a relative path to your source file(s).
             src/main/cpp/gpssim.c
             src/main/cpp/lime_main.c
             src/main/cpp/getRTCM3Eph.c
             src/main/cpp/native-lib.cpp
             src/main/cpp/hello-jnicallback.c)

# Searches for a specified prebuilt library and stores the path as a
# variable. Because CMake includes system libraries in the search path by
# default, you only need to specify the name of the public NDK library
# you want to add. CMake verifies that the library exists before
# completing its build.

find_library( # Sets the name of the path variable.
              log-lib

              # Specifies the name of the NDK library that
              # you want CMake to locate.
              log )

# Specifies libraries CMake should link to your target library. You
# can link multiple libraries, such as libraries you define in this
# build script, prebuilt third-party libraries, or system libraries.

target_link_libraries( # Specifies the target library.
                       native-lib

                       # Links the target library to the log library
                       # included in the NDK.
                       ${log-lib})