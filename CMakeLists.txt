cmake_minimum_required(VERSION 3.15.0)

cmake_policy(SET CMP0091 NEW)

project(
  chopt
  VERSION 1.2.3
  DESCRIPTION "A program to generate optimal Star Power paths for Clone Hero"
  LANGUAGES CXX)

include(cmake/Sanitisers.cmake)

# Require standard C++17
function(set_cpp_standard target)
  set_target_properties(
    ${target}
    PROPERTIES CXX_STANDARD 17
               CXX_STANDARD_REQUIRED ON
               CXX_EXTENSIONS OFF)
endfunction()

# Set warning flags
function(set_warnings target)
  target_compile_options(
    ${target}
    PRIVATE $<$<CXX_COMPILER_ID:Clang,AppleClang,GNU>:
            -Wall
            -Wextra
            -Werror
            -Wno-c++98-compat>
            $<$<CXX_COMPILER_ID:MSVC>:
            /W4
            /WX>)
endfunction()

# Link a library without throwing warnings during compilation. From
# https://stackoverflow.com/a/61933129/13212204
function(target_link_libraries_system target scope)
  set(libs ${ARGN})
  foreach(lib ${libs})
    get_target_property(lib_include_dirs ${lib} INTERFACE_INCLUDE_DIRECTORIES)
    target_include_directories(${target} SYSTEM ${scope} ${lib_include_dirs})
    target_link_libraries(${target} ${scope} ${lib})
  endforeach()
endfunction()

find_package(PNG REQUIRED)

include(FetchContent)

FetchContent_Declare(
  Argparse
  GIT_REPOSITORY "https://github.com/p-ranav/argparse"
  GIT_TAG "473d550ea313ea315a820b3b1b426254ad17061a")

FetchContent_Declare(
  Nowide
  GIT_REPOSITORY "https://github.com/boostorg/nowide"
  GIT_TAG "02f40f0b5f5686627fcddae93ff88ca399db4766")

FetchContent_MakeAvailable(Argparse)
FetchContent_MakeAvailable(Nowide)

# We do not care about building nowide's tests
set(BUILD_TESTING OFF)
set_property(TARGET nowide PROPERTY MSVC_RUNTIME_LIBRARY
                                    "MultiThreaded$<$<CONFIG:Debug>:DebugDLL>")

cmake_policy(SET CMP0083 NEW)
include(CheckPIESupported)
check_pie_supported(OUTPUT_VARIABLE output LANGUAGES CXX)
if(NOT CMAKE_CXX_LINK_NO_PIE_SUPPORTED)
  message(WARNING "No PIE is not supported at link time: ${output}.\n"
                  "PIE link options will not be passed to linker.")
endif()

add_executable(
  chopt
  src/main.cpp
  src/chart.cpp
  src/image.cpp
  src/imagebuilder.cpp
  src/ini.cpp
  src/midi.cpp
  src/optimiser.cpp
  src/points.cpp
  src/processed.cpp
  src/settings.cpp
  src/song.cpp
  src/songparts.cpp
  src/sp.cpp
  src/stringutil.cpp
  src/time.cpp
  resources/chopt.exe.manifest
  resources/resources.rc)
target_include_directories(
  chopt PRIVATE "${PROJECT_SOURCE_DIR}/include" "${PROJECT_SOURCE_DIR}/libs"
                ${PNG_INCLUDE_DIRS})
target_link_libraries_system(chopt PRIVATE argparse nowide)
target_link_libraries(chopt PRIVATE ${PNG_LIBRARIES})

set_property(TARGET chopt PROPERTY POSITION_INDEPENDENT_CODE FALSE)
set_property(TARGET chopt PROPERTY MSVC_RUNTIME_LIBRARY
                                   "MultiThreaded$<$<CONFIG:Debug>:DebugDLL>")
set_cpp_standard(chopt)
set_warnings(chopt)
enable_sanitisers(chopt)

option(BUILD_GUI "Build the GUI version" ON)
if(BUILD_GUI)
  find_package(
    Qt6
    COMPONENTS Widgets
    REQUIRED)

  set(CMAKE_AUTOMOC ON)
  set(CMAKE_AUTORCC ON)
  set(CMAKE_AUTOUIC ON)
  set(CMAKE_INCLUDE_CURRENT_DIR ON)

  add_executable(
    choptgui WIN32
    gui/main.cpp
    gui/mainwindow.cpp
    gui/mainwindow.ui
    src/chart.cpp
    src/image.cpp
    src/imagebuilder.cpp
    src/ini.cpp
    src/midi.cpp
    src/optimiser.cpp
    src/points.cpp
    src/processed.cpp
    src/song.cpp
    src/songparts.cpp
    src/sp.cpp
    src/stringutil.cpp
    src/time.cpp
    resources/choptgui.exe.manifest
    resources/resources.qrc
    resources/resources.rc)
  target_include_directories(
    choptgui PRIVATE "${PROJECT_SOURCE_DIR}/include"
                     "${PROJECT_SOURCE_DIR}/libs" ${PNG_INCLUDE_DIRS})
  target_link_libraries_system(choptgui PRIVATE argparse nowide)
  target_link_libraries(choptgui PRIVATE ${PNG_LIBRARIES} Qt6::Widgets)

  set_property(TARGET choptgui PROPERTY POSITION_INDEPENDENT_CODE FALSE)
  set_property(
    TARGET choptgui PROPERTY MSVC_RUNTIME_LIBRARY
                             "MultiThreaded$<$<CONFIG:Debug>:DebugDLL>")
  set_cpp_standard(choptgui)
endif()

include(CTest)

option(PACKAGE_TESTS "Build the tests" ON)
if(PACKAGE_TESTS)
  enable_testing()
  add_subdirectory(tests)
endif()

option(BUILD_FUZZ_TARGETS "Build the fuzzing targets" OFF)
if(BUILD_FUZZ_TARGETS)
  add_subdirectory(fuzzing_targets)
endif()

option(ENABLE_LTO "Enable Link Time Optimisation" OFF)
if(ENABLE_LTO)
  cmake_policy(SET CMP0069 NEW)

  include(CheckIPOSupported)
  check_ipo_supported(
    RESULT supported
    OUTPUT output
    LANGUAGES CXX)
  if(supported)
    set_property(TARGET chopt PROPERTY INTERPROCEDURAL_OPTIMIZATION TRUE)
    if(BUILD_GUI)
      set_property(TARGET choptgui PROPERTY INTERPROCEDURAL_OPTIMIZATION TRUE)
    endif()
  else()
    message(WARNING "LTO is not supported: ${output}")
  endif()
endif()
