cmake_minimum_required(VERSION 3.20)
project(infinity-settings VERSION 0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)

find_package(Qt6 REQUIRED COMPONENTS Core Gui Qml Quick QuickControls2)

qt_add_library(infinity_core STATIC
  src/Paths.cpp
  src/SettingsStore.cpp
  src/Theme.cpp
  src/LuaGenerator.cpp
  src/Templates.cpp
  src/Applier.cpp
)
target_include_directories(infinity_core PUBLIC src)
target_link_libraries(infinity_core PUBLIC Qt6::Core)

qt_add_executable(infinity-settings src/main.cpp)
target_link_libraries(infinity-settings PRIVATE infinity_core)

install(TARGETS infinity-settings RUNTIME DESTINATION bin)
