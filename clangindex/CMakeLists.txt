cmake_minimum_required(VERSION 3.10)
project(SQLiteExample)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Set CMake policy to ensure CMAKE_REQUIRED_LIBRARIES is honored
if(POLICY CMP0075)
    cmake_policy(SET CMP0075 NEW)
endif()

# Dependencies:

# Include the subdirectory containing Protocol Buffers' CMake project
add_subdirectory(extern/protobuf)
include_directories(extern/protobuf/src)

# Define our target
add_executable(SQLiteExample src/main.cpp)

# Assuming sqlite3 is installed and findable in standard system paths
find_package(SQLite3 REQUIRED)

target_link_libraries(SQLiteExample PRIVATE SQLite::SQLite3)

if(APPLE)
    # Assuming llvm was installed using Homebrew
    execute_process(COMMAND brew --prefix llvm OUTPUT_VARIABLE LLVM_PREFIX OUTPUT_STRIP_TRAILING_WHITESPACE)
    list(APPEND CMAKE_PREFIX_PATH "${LLVM_PREFIX}")
endif()

# Find LLVM/Clang packages
find_package(Clang REQUIRED CONFIG)

# Include headers
include_directories(${CLANG_INCLUDE_DIRS})
add_definitions(${CLANG_DEFINITIONS})

# TODO: Properly implement this. This is the only way I was able to make this work
target_link_libraries(SQLiteExample PRIVATE
    "${LLVM_PREFIX}/lib/libclang.dylib")  # or .a if you prefer static linking

# Protobuf

find_package(Protobuf REQUIRED)
message(STATUS "Using protobuf ${protobuf_VERSION}")

protobuf_generate_cpp(PROTO_SRCS PROTO_HDRS ../proto/functioninfo.proto)

# Include generated *.pb.h files
include_directories(${CMAKE_CURRENT_BINARY_DIR})

# Add the generated sources to the target
target_sources(SQLiteExample PRIVATE ${PROTO_SRCS} ${PROTO_HDRS})

include_directories(${Protobuf_INCLUDE_DIRS})
target_link_libraries(SQLiteExample PRIVATE libprotobuf)

find_library(ABSL_BASE absl_base PATHS ${ABSEIL_PREFIX}/lib REQUIRED)
find_library(ABSL_STRINGS absl_strings PATHS ${ABSEIL_PREFIX}/lib REQUIRED)
find_library(ABSL_SYNCHRONIZATION absl_synchronization PATHS ${ABSEIL_PREFIX}/lib REQUIRED)
target_link_libraries(SQLiteExample PRIVATE
    ${ABSL_BASE}
    ${ABSL_STRINGS}
    ${ABSL_SYNCHRONIZATION}
)
