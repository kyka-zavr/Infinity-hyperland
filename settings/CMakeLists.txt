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

set_source_files_properties(qml/Style.qml PROPERTIES QT_QML_SINGLETON_TYPE TRUE)

qt_add_executable(infinity-settings src/main.cpp src/Backend.cpp src/Backend.hpp)
qt_add_qml_module(infinity-settings
  URI Infinity
  VERSION 1.0
  QML_FILES
    qml/Main.qml qml/Style.qml qml/Btn.qml qml/Toggle.qml qml/Segmented.qml qml/Dropdown.qml qml/Field.qml qml/Slide.qml
    qml/SettingRow.qml qml/Card.qml qml/KeyCapture.qml qml/ColorField.qml
    qml/PageInfinity.qml qml/PageKeyboard.qml qml/PageAppearance.qml qml/PageBinds.qml qml/PageSystem.qml
)
target_link_libraries(infinity-settings PRIVATE infinity_core Qt6::Gui Qt6::Qml Qt6::Quick Qt6::QuickControls2)

install(TARGETS infinity-settings RUNTIME DESTINATION bin)
