cmake_minimum_required(VERSION 3.0)

project(nodesetLoader)

include(GNUInstallDirs)

set(CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake)

option(ENABLE_CONAN "use conan for consuming dependencies" on)
option(ENABLE_TESTING "enable tests" off)
option(ENABLE_BACKEND_OPEN62541 "backend for open62541" off)
option(ENABLE_ASAN "build with address sanitizer enabled" off)
option(ENABLE_INTEGRATION_TEST "run detailled tests to compare address spaces" off)
option(ENABLE_DATATYPEIMPORT_TEST "run tests for importing datatypes" off)
option(CALC_COVERAGE "calculate code coverage" off)
option(USE_MEMBERTYPE_INDEX "necessary for open62541 backend with version <= 1.2.x" ON)


if(${ENABLE_CONAN})
    if(NOT EXISTS "${CMAKE_BINARY_DIR}/conan.cmake")
    message(STATUS "Downloading conan.cmake from https://github.com/conan-io/cmake-conan")
    file(DOWNLOAD "https://github.com/conan-io/cmake-conan/raw/v0.15/conan.cmake"
                    "${CMAKE_BINARY_DIR}/conan.cmake")
    endif()

    include(${CMAKE_BINARY_DIR}/conan.cmake)
    conan_cmake_run(CONANFILE conanfile.txt
                    BASIC_SETUP
                    CMAKE_TARGETS
                    BUILD missing)

    include(${CMAKE_BINARY_DIR}/conan_paths.cmake)
endif()

#link against this library to gather coverage info
add_library(coverageLib INTERFACE)

if(${CALC_COVERAGE})
    #set(GCOV_COMPILE_OPTIONS -g -O0 -fno-inline -fno-inline-small-functions -fno-default-inline -fprofile-arcs -ftest-coverage)

    message(STATUS "Code coverage is enabled.")
    # Note that --coverage is synonym for the necessary compiler and
    # linker flags for the given compiler.  For example, with GCC,
    # --coverage translates to -fprofile-arcs -ftest-coverage when
    # compiling and -lgcov when linking
    #add_compile_options(--coverage -O0)
    #add_link_options(--coverage)
    target_compile_options(coverageLib INTERFACE
        -O0        # no optimization
        -g         # generate debug info
        --coverage # sets all required flags)
    )
    target_link_libraries(coverageLib INTERFACE --coverage)
endif()

if(MSVC)
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /W3 /WX /w44996") # Compiler warnings, error on warning
endif()

if(CMAKE_COMPILER_IS_GNUCC OR "x${CMAKE_C_COMPILER_ID}" STREQUAL "xClang")
    set(C_COMPILE_DEFS -std=c99 -pipe -Wall -Wextra -Wpedantic -Werror
                        -Wno-unused-parameter
                        -Wmissing-prototypes -Wstrict-prototypes -Wredundant-decls
                        -Wformat -Wformat-security -Wformat-nonliteral
                        -Wuninitialized -Winit-self
                        -Wcast-qual
                        -Wstrict-overflow
                        -Wnested-externs
                        -Wmultichar
                        -Wundef
                        -Wc++-compat
                        -Wsign-conversion
                        -Wconversion
                        -Wshadow
                        -fexceptions
                        -Wswitch-enum)

    set(PTHREAD_LIB pthread)
endif()

if(${ENABLE_BACKEND_OPEN62541})
    find_package(open62541 REQUIRED)
endif()
find_package(LibXml2 REQUIRED)

add_library(NodesetLoader
    src/NodesetLoader.c 
    src/Sort.c 
    src/Nodeset.c 
    src/CharAllocator.c 
    src/AliasList.c 
    src/NamespaceList.c
    src/nodes/Node.c
    src/nodes/DataTypeNode.c
    src/nodes/NodeContainer.c
    src/nodes/InstanceNode.c
    src/NodeId.c
    src/PrintfLogger.c
    src/Value.c
    src/InternalRefService.c
    src/Parser.c)

target_include_directories(NodesetLoader
    PUBLIC  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
            $<INSTALL_INTERFACE:include>
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src ${LIBXML2_INCLUDE_DIRS})
target_link_libraries(NodesetLoader PRIVATE ${LIBXML2_LIBRARIES})
if(${CALC_COVERAGE})
    target_link_libraries(NodesetLoader PUBLIC coverageLib)
endif()

if(${ENABLE_BACKEND_OPEN62541})
    target_include_directories(NodesetLoader PRIVATE open62541::open62541)
    target_link_libraries(NodesetLoader PRIVATE open62541::open62541)
    if(${USE_MEMBERTYPE_INDEX})
        target_compile_definitions(NodesetLoader PUBLIC -DUSE_MEMBERTYPE_INDEX=1)
    endif()
endif()

target_compile_options(NodesetLoader PRIVATE ${C_COMPILE_DEFS})
set_target_properties(NodesetLoader PROPERTIES C_VISIBILITY_PRESET hidden)
if(${ENABLE_ASAN})
    target_link_libraries(NodesetLoader INTERFACE "-g -fno-omit-frame-pointer -fsanitize=address -fsanitize-address-use-after-scope -fsanitize-coverage=trace-pc-guard,trace-cmp -fsanitize=leak -fsanitize=undefined")
endif()

if(ENABLE_TESTING)
    find_package(Check REQUIRED)
    include(CTest)
    enable_testing()
    add_subdirectory(tests)
endif()
add_subdirectory(backends)

if(${CALC_COVERAGE})
    add_subdirectory(coverage)
endif()

#install
if(NOT ${CALC_COVERAGE})
    set(NODESETLOADER_PUBLIC_HEADER 
        ${PROJECT_SOURCE_DIR}/include/NodesetLoader/NodesetLoader.h
        ${PROJECT_SOURCE_DIR}/include/NodesetLoader/NodeId.h
        ${PROJECT_SOURCE_DIR}/include/NodesetLoader/Logger.h
        ${PROJECT_SOURCE_DIR}/include/NodesetLoader/arch.h
        ${PROJECT_SOURCE_DIR}/include/NodesetLoader/ReferenceService.h
        ${PROJECT_SOURCE_DIR}/include/NodesetLoader/Extension.h
    )

    if(${ENABLE_BACKEND_OPEN62541})
        list(APPEND NODESETLOADER_PUBLIC_HEADER ${CMAKE_CURRENT_SOURCE_DIR}/backends/open62541/include/NodesetLoader/backendOpen62541.h)
        list(APPEND NODESETLOADER_PUBLIC_HEADER ${CMAKE_CURRENT_SOURCE_DIR}/backends/open62541/include/NodesetLoader/dataTypes.h)
    endif()

    set_target_properties(NodesetLoader PROPERTIES PUBLIC_HEADER "${NODESETLOADER_PUBLIC_HEADER}")

    install(TARGETS NodesetLoader
            EXPORT NodesetLoader
            LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
            ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
            RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
            PUBLIC_HEADER DESTINATION include/NodesetLoader)
    install(FILES nodesetloader-config.cmake DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/NodesetLoader)

    install(EXPORT NodesetLoader DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/NodesetLoader)
endif()
