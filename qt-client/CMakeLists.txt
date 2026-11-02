cmake_minimum_required(VERSION 3.21)

project(IntelligentLabReservationQt VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

find_package(Qt6 6.4 REQUIRED COMPONENTS Core Gui Widgets)

qt_add_executable(IntelligentLabReservationQt
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
endif()
