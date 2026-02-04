cmake_minimum_required(VERSION 3.15)
project(seko)

set(CMAKE_CXX_STANDARD 17)

# Enable Windows subsystem (no console)
if(WIN32)
    set(CMAKE_WIN32_EXECUTABLE TRUE)
    # Force Unicode build
    add_definitions(-DUNICODE -D_UNICODE)
endif()

add_executable(seko WIN32
    src/main.cpp
    src/upload.cpp
    src/overlay.cpp
    src/toast.cpp
    src/settings.cpp
    src/resource.rc
)

target_link_libraries(seko
    gdiplus
    winhttp
    shell32
    comctl32
)

# Set application icon
set_target_properties(seko PROPERTIES
    WIN32_EXECUTABLE TRUE
)
