cmake_minimum_required(VERSION 3.24)
project(test)

set(CMAKE_CXX_STANDARD 23)

# Create the executable
add_executable(test main.cpp)

# Link against the DirectShow libraries
target_link_libraries(test strmiids mfplat mf mfreadwrite mfuuid)
