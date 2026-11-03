cmake_minimum_required(VERSION 3.21)

project(IntelligentLabReservationQt VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

# Keep Chinese UI literals portable on Windows compiler installations.
if(MSVC)
    add_compile_options(/utf-8)
endif()

find_package(Qt6 6.4 REQUIRED COMPONENTS Core Gui Widgets)

qt_add_executable(IntelligentLabReservationQt
    resources.qrc
    src/main.cpp
    src/domain/Types.h
    src/services/LabRepository.h
    src/services/LabRepository.cpp
    src/ui/MainWindow.h
    src/ui/MainWindow.cpp
)

target_link_libraries(IntelligentLabReservationQt PRIVATE Qt6::Core Qt6::Gui Qt6::Widgets)
target_include_directories(IntelligentLabReservationQt PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)

set_target_properties(IntelligentLabReservationQt PROPERTIES
    WIN32_EXECUTABLE TRUE
)

include(CTest)
if(BUILD_TESTING)
    qt_add_executable(LabRepositoryCheck
        tests/RepositoryCheck.cpp
        src/domain/Types.h
        src/services/LabRepository.h
        src/services/LabRepository.cpp
    )
    target_include_directories(LabRepositoryCheck PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
    target_link_libraries(LabRepositoryCheck PRIVATE Qt6::Core)
    add_test(NAME LabRepositoryCheck COMMAND LabRepositoryCheck)

    qt_add_executable(LabWindowCheck
        tests/WindowCheck.cpp
        resources.qrc
        src/domain/Types.h
        src/services/LabRepository.h
        src/services/LabRepository.cpp
        src/ui/MainWindow.h
        src/ui/MainWindow.cpp
    )
    target_include_directories(LabWindowCheck PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
    target_link_libraries(LabWindowCheck PRIVATE Qt6::Core Qt6::Gui Qt6::Widgets)
    add_test(NAME LabWindowCheck COMMAND LabWindowCheck)
    set_tests_properties(LabWindowCheck PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")
endif()
