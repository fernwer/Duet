cmake_minimum_required(VERSION 3.10)
project(LumosKernel CXX C)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
add_compile_options(-Wno-register -Wno-unused-parameter)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

find_program(PG_CONFIG_EXECUTABLE pg_config)

if(NOT PG_CONFIG_EXECUTABLE)
    message(FATAL_ERROR "pg_config not found! Please install PostgreSQL development package.")
endif()

execute_process(
    COMMAND ${PG_CONFIG_EXECUTABLE} --includedir-server
    OUTPUT_VARIABLE PG_INC_SERVER
    OUTPUT_STRIP_TRAILING_WHITESPACE
)
execute_process(
    COMMAND ${PG_CONFIG_EXECUTABLE} --libdir
    OUTPUT_VARIABLE PG_LIB_DIR
    OUTPUT_STRIP_TRAILING_WHITESPACE
)

message(STATUS "PostgreSQL Server Headers: ${PG_INC_SERVER}")
message(STATUS "PostgreSQL Library Directory: ${PG_LIB_DIR}")

find_package(Protobuf REQUIRED)

set(PROTO_FILES ${CMAKE_CURRENT_SOURCE_DIR}/protos/mqo.proto)
protobuf_generate_cpp(PROTO_SRCS PROTO_HDRS ${PROTO_FILES})

set(SOURCE_FILES
    src/pg_entry.cpp
    src/lumos_kernel.cpp
    src/exec/executor.cpp
    src/exec/type_mapper.cpp
    src/exec/planner.cpp
    src/exec/runtime.cpp
    ${PROTO_SRCS}
)


add_library(lumos_kernel MODULE ${SOURCE_FILES} ${PROTO_HDRS})

target_include_directories(lumos_kernel
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${CMAKE_CURRENT_SOURCE_DIR}/include/exec
        ${PG_INC_SERVER}
        ${Protobuf_INCLUDE_DIRS}
        ${CMAKE_CURRENT_BINARY_DIR}
)

target_link_libraries(lumos_kernel
    PRIVATE
        ${Protobuf_LIBRARIES}
)

link_directories(${PG_LIB_DIR})

set_target_properties(lumos_kernel PROPERTIES PREFIX "")
set_target_properties(lumos_kernel PROPERTIES SUFFIX ".so")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/lib)

message(STATUS "Output Directory: ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}")