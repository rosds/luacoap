cmake_minimum_required (VERSION 3.5)
project (luacoap C)

include (ExternalProject)

set (luacoap_VERSION_MAJOR 0)
set (luacoap_VERSION_MINOR 1)
set (luacoap_VERSION_PATCH 0)

ExternalProject_Add(smcp
  GIT_REPOSITORY https://github.com/darconeous/smcp
  GIT_TAG "0.6.5"
  SOURCE_DIR ${CMAKE_CURRENT_BINARY_DIR}/smcp
  CMAKE_ARGS -DCMAKE_INSTALL_PREFIX=<INSTALL_DIR>
  CONFIGURE_COMMAND ${CMAKE_CURRENT_BINARY_DIR}/smcp/bootstrap.sh &&
  ${CMAKE_CURRENT_BINARY_DIR}/smcp/configure --prefix=<INSTALL_DIR>
  BUILD_COMMAND ${MAKE}
  UPDATE_COMMAND ""
)
ExternalProject_Get_Property(smcp install_dir)
set (libsmcp ${install_dir}/lib/libsmcp.so)
include_directories (${install_dir}/include)

set (CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_CURRENT_SOURCE_DIR}/cmake/modules/")
find_package (Lua 5.3 REQUIRED)
include_directories (${LUA_INCLUDE_DIR})

find_package (Threads REQUIRED)

## Source files
aux_source_directory (src src_files) 

include_directories (include)
add_library (coap SHARED ${src_files})
set_target_properties (coap PROPERTIES PREFIX "")
set_target_properties (coap PROPERTIES CMAKE_POSITION_INDEPENDENT_CODE True)

set(LINK_FLAGS ${LINK_FLAGS} "-Wl,-whole-archive")
target_link_libraries (coap ${libsmcp} ${LUA_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT})
add_dependencies (coap smcp)

install (FILES ${CMAKE_BINARY_DIR}/coap.so DESTINATION lib/lua/5.3)

set (CPACK_GENERATOR "DEB")
set (CPACK_PACKAGE_NAME ${CMAKE_PROJECT_NAME})
set (CPACK_SET_DESTDIR TRUE)
set (CPACK_DEBIAN_PACKAGE_MAINTAINER "Alfonso Ros")
set (CPACK_PACKAGE_VERSION_MAJOR "0")
set (CPACK_PACKAGE_VERSION_MINOR "1")
set (CPACK_PACKAGE_VERSION_PATCH "0")
set (CPACK_OUTPUT_FILE_PREFIX "${CMAKE_SOURCE_DIR}/downloads")
include (CPack)
