cmake_minimum_required(VERSION 3.15)

project(FreecamAnchor CXX)

# Find latest version at https://github.com/OrfeasZ/ZHMModSDK/releases
# Set ZHMMODSDK_DIR variable to a local directory to use a local copy of the ZHMModSDK.
set(ZHMMODSDK_VER "v4.0.0-rc.2")
include(cmake/setup-zhmmodsdk.cmake)

# Set C++ standard to C++23.
set(CMAKE_CXX_STANDARD 23)

# Create the FreecamAnchor mod library.
add_library(FreecamAnchor SHARED
    src/FreecamAnchor.cpp
    src/FreecamAnchor.h
)

# Add SDK and DX headers dependencies.
find_package(directx-headers CONFIG REQUIRED)

target_link_libraries(FreecamAnchor
    ZHMModSDK
    Microsoft::DirectX-Guids
    Microsoft::DirectX-Headers
)

install(TARGETS FreecamAnchor
    RUNTIME DESTINATION bin
)

# Install the mod to the game folder when the `GAME_INSTALL_PATH` variable is set.
zhmmodsdk_install(FreecamAnchor)
