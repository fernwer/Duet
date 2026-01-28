cmake_minimum_required(VERSION 3.10)
project(lumos_proxy)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_compile_options(-O3 -Wno-unused-variable -Wno-unused-but-set-variable)

find_program(MAKE_EXE NAMES make gmake REQUIRED)

option(PROXY_VERBOSE "Enable verbose logging" ON)

find_package(Protobuf REQUIRED)
find_package(PostgreSQL REQUIRED) # libpq
find_package(Threads REQUIRED)

set(LIBPG_QUERY_DIR "${CMAKE_CURRENT_SOURCE_DIR}/third_party/libpg_query")
set(LIBPG_QUERY_STATIC_LIB "${LIBPG_QUERY_DIR}/libpg_query.a")

include_directories(${LIBPG_QUERY_DIR}) 

add_custom_command(
    OUTPUT ${LIBPG_QUERY_STATIC_LIB}
    COMMAND ${MAKE_EXE} build
    WORKING_DIRECTORY ${LIBPG_QUERY_DIR}
    COMMENT "Compiling libpg_query C library ..."
    VERBATIM
)
add_custom_target(ensure_libpg_query DEPENDS ${LIBPG_QUERY_STATIC_LIB})

add_library(libpg_query STATIC IMPORTED GLOBAL)
set_target_properties(libpg_query PROPERTIES IMPORTED_LOCATION "${LIBPG_QUERY_STATIC_LIB}")
add_dependencies(libpg_query ensure_libpg_query)

set(ORIGINAL_PROTO "${LIBPG_QUERY_DIR}/protobuf/pg_query.proto")
set(SHADOW_PROTO "${CMAKE_CURRENT_BINARY_DIR}/pg_query_cpp.proto")

add_custom_command(
    OUTPUT ${SHADOW_PROTO}
    COMMAND sed "s/package pg_query;/package pg_query_cpp;/g" ${ORIGINAL_PROTO} > ${SHADOW_PROTO}
    DEPENDS ${ORIGINAL_PROTO}
    COMMENT "Creating shadow proto file..."
    VERBATIM
)
protobuf_generate_cpp(PARSER_PROTO_SRCS PARSER_PROTO_HDRS "${SHADOW_PROTO}")

set(MQO_PROTO "${CMAKE_CURRENT_SOURCE_DIR}/protos/mqo.proto")
protobuf_generate_cpp(MQO_PROTO_SRCS MQO_PROTO_HDRS "${MQO_PROTO}")

file(GLOB CORE_SOURCES "src/*.cpp")

list(FILTER CORE_SOURCES EXCLUDE REGEX ".*main\\.cpp$")

add_library(lumos_core STATIC 
    ${CORE_SOURCES} 
    ${PARSER_PROTO_SRCS} 
    ${MQO_PROTO_SRCS}
)

target_include_directories(lumos_core PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${Protobuf_INCLUDE_DIRS}
    ${PostgreSQL_INCLUDE_DIRS}
    ${CMAKE_CURRENT_BINARY_DIR}
)

target_link_libraries(lumos_core PUBLIC
    libpg_query
    ${Protobuf_LIBRARIES}
    ${PostgreSQL_LIBRARIES}
    Threads::Threads
)

target_compile_definitions(lumos_core PUBLIC PROXY_VERBOSE=$<BOOL:${PROXY_VERBOSE}>)

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/src/main.cpp")
    add_executable(lumos_proxy src/main.cpp)
    target_link_libraries(lumos_proxy PRIVATE lumos_core)
endif()