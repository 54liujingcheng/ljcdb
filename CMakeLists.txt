cmake_minimum_required(VERSION 3.20)

project(ljcdb LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Build only the bundled ANTLR C++ runtime library.  The generated SQL parser
# is already checked into parser/generated, so Java is not needed to build the
# database executable.
set(WITH_DEMO OFF CACHE BOOL "Build ANTLR demo" FORCE)
set(ANTLR_BUILD_CPP_TESTS OFF CACHE BOOL "Build ANTLR runtime tests" FORCE)
set(ANTLR_BUILD_SHARED OFF CACHE BOOL "Build shared ANTLR runtime" FORCE)
set(ANTLR_BUILD_STATIC ON CACHE BOOL "Build static ANTLR runtime" FORCE)
set(ANTLR4_INSTALL OFF CACHE BOOL "Install ANTLR runtime" FORCE)

add_subdirectory(
    third_party/antlr4-cpp-runtime-4.13.2-source
    antlr4-runtime
    EXCLUDE_FROM_ALL
)

add_executable(ljcdb
    main.cpp
    execute.cpp
    query.cpp
    system_management.cpp
    parser/generated/SQLLexer.cpp
    parser/generated/SQLParser.cpp
    parser/generated/SQLBaseVisitor.cpp
    parser/generated/SQLVisitor.cpp
)

target_include_directories(ljcdb PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/parser/generated
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party/antlr4-cpp-runtime-4.13.2-source/runtime/src
)

target_link_libraries(ljcdb PRIVATE antlr4_static)

set_target_properties(ljcdb PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)

if(MSVC)
    target_compile_options(ljcdb PRIVATE /W4)
else()
    target_compile_options(ljcdb PRIVATE -Wall -Wextra)
endif()
