cmake_minimum_required(VERSION 3.21)
project(settings LANGUAGES C CXX)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_CXX_STANDARD 17)
find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets)

get_filename_component(PUREC_ROOT "${CMAKE_CURRENT_LIST_DIR}/../.." ABSOLUTE)

add_executable(settings
    src/main.cpp
    src/ui.cpp src/ui.h
    src/window.cpp src/window.h
    src/pages.h
    src/page_about.cpp
    src/page_network.cpp
    src/page_sound.cpp
    src/page_display.cpp
    src/page_appearance.cpp
    src/page_storage.cpp
    src/page_datetime.cpp
    src/page_power.cpp
    src/sysapi.c src/sysapi.h
    assets/icons/icons.qrc)
target_include_directories(settings PRIVATE
    ${PUREC_ROOT}/src
    ${PUREC_ROOT}/video-backend/include)
target_link_libraries(settings PRIVATE Qt6::Widgets Qt6::QPurecIntegrationPlugin)
