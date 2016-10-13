cmake_minimum_required (VERSION 3.5)

include (ExternalProject)

ExternalProject_Add(smcp
  GIT_REPOSITORY https://github.com/darconeous/smcp
  GIT_TAG "0.6.5"
  SOURCE_DIR ${CMAKE_CURRENT_BINARY_DIR}/smcp
  CONFIGURE_COMMAND ${CMAKE_CURRENT_BINARY_DIR}/smcp/bootstrap.sh && ${CMAKE_CURRENT_BINARY_DIR}/smcp/configure
  BUILD_COMMAND ${MAKE}
  UPDATE_COMMAND ""
  INSTALL_COMMAND ""
)

set (libsmcp "${CMAKE_CURRENT_BINARY_DIR}/smcp/.libs/libsmcp.so")

add_executable (luacoap src/main.cpp)
target_link_libraries (luacoap ${libsmcp})
add_dependencies (luacoap smcp)
