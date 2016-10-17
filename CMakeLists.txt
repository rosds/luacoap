cmake_minimum_required (VERSION 3.5)
project (luacoap C)

include (ExternalProject)

ExternalProject_Add(smcp
  GIT_REPOSITORY https://github.com/darconeous/smcp
  GIT_TAG "0.6.5"
  SOURCE_DIR ${CMAKE_CURRENT_BINARY_DIR}/smcp
  INSTALL_DIR ${CMAKE_CURRENT_BINARY_DIR}/smcp/install
  CMAKE_ARGS -DCMAKE_INSTALL_PREFIX=<INSTALL_DIR>
  CONFIGURE_COMMAND ${CMAKE_CURRENT_BINARY_DIR}/smcp/bootstrap.sh && ${CMAKE_CURRENT_BINARY_DIR}/smcp/configure --prefix=<INSTALL_DIR>
  BUILD_COMMAND ${MAKE}
  UPDATE_COMMAND ""
)
ExternalProject_Get_Property(smcp install_dir)
set (libsmcp ${install_dir}/lib/libsmcp.so)
include_directories (${install_dir}/include)

set (CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_CURRENT_SOURCE_DIR}/cmake/modules/")
find_package (Lua 5.2 REQUIRED)
include_directories (${LUA_INCLUDE_DIR})

include_directories (include)
add_library (luacoap SHARED src/luacoap.c src/client/get.c)

target_link_libraries (luacoap ${libsmcp} ${LUA_LIBRARIES})
add_dependencies (luacoap smcp)
