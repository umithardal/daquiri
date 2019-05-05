
# git info
execute_process(COMMAND "git" "symbolic-ref" "--short" "HEAD"
  WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
  OUTPUT_VARIABLE GIT_BRANCH
  OUTPUT_STRIP_TRAILING_WHITESPACE)
execute_process(COMMAND "git" "rev-parse" "HEAD"
  WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
  OUTPUT_VARIABLE GIT_SHA1
  OUTPUT_STRIP_TRAILING_WHITESPACE)
execute_process(COMMAND "git" "rev-parse" "--short" "HEAD"
  WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
  OUTPUT_VARIABLE GIT_HASH
  OUTPUT_STRIP_TRAILING_WHITESPACE)

# hostname
cmake_host_system_information(RESULT HOSTNAME QUERY HOSTNAME)

# user
# todo: test on Windows
set(USERNAME "$ENV{USER}")

# Build time in UTC ISO 8601
string(TIMESTAMP UTC_TIMESTAMP UTC)

# OS info
set(SYSTEM "${CMAKE_SYSTEM}")
set(PROCESSOR "${CMAKE_SYSTEM_PROCESSOR}")

add_definitions(-DBI_GIT_BRANCH="${GIT_BRANCH}")
add_definitions(-DBI_GIT_SHA1="${GIT_SHA1}")
add_definitions(-DBI_GIT_HASH="${GIT_HASH}")
add_definitions(-DBI_HOSTNAME="${HOSTNAME}")
add_definitions(-DBI_USERNAME="${USERNAME}")
add_definitions(-DBI_CMAKE_TIME="${UTC_TIMESTAMP}")
add_definitions(-DBI_SYSTEM="${SYSTEM}")
add_definitions(-DBI_PROCESSOR="${PROCESSOR}")

file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/build_info)
file(WRITE ${CMAKE_BINARY_DIR}/build_info/build_time.cmake "STRING(TIMESTAMP TIMEZ UTC)\n")
file(APPEND ${CMAKE_BINARY_DIR}/build_info/build_time.cmake "FILE(WRITE ${CMAKE_BINARY_DIR}/build_info/build_time.h \"#pragma once\\n\")\n")
file(APPEND ${CMAKE_BINARY_DIR}/build_info/build_time.cmake "FILE(APPEND ${CMAKE_BINARY_DIR}/build_info/build_time.h \"#define BI_BUILD_TIME \\\"\${TIMEZ}\\\"\\n\\n\")\n")
add_custom_target(
  build_time
  COMMAND ${CMAKE_COMMAND} -P ${CMAKE_BINARY_DIR}/build_info/build_time.cmake
  ADD_DEPENDENCIES ${CMAKE_BINARY_DIR}/build_info/build_time.cmake)
