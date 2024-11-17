cmake_minimum_required(VERSION 3.21)

project(UntitledDBusUtils VERSION 1.2.0.0)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_C_STANDARD 99)

include_directories(.)

set(UDBUS_LIBRARY_TYPE "SHARED")
if (BUILD_VARIANT_STATIC)
    set(UDBUS_LIBRARY_TYPE "STATIC")
endif()

find_package(PkgConfig REQUIRED)
pkg_check_modules(DBUS REQUIRED dbus-1)

link_directories(${DBUS_LIBRARY_DIRS})

set(UDBUS_HEADERS "DBusUtils.hpp" "DBusUtilsMeta.hpp")

add_library(UntitledDBusUtils ${UDBUS_LIBRARY_TYPE} Connection.cpp DBusUtils.cpp Error.cpp Iterator.cpp Message.cpp
        PendingCall.cpp ArrayBuilder.cpp MessageAppend.cpp MessageGet.cpp ${UDBUS_HEADERS})

include_directories(${DBUS_INCLUDE_DIRS})
target_include_directories(UntitledDBusUtils PUBLIC ${DBUS_INCLUDE_DIRS})
target_compile_definitions(UntitledDBusUtils PUBLIC UIMGUI_DBUS_SUBMODULE_ENABLED)

list(APPEND compile_defs "UIMGUI_DBUS_SUBMODULE_ENABLED")
foreach (A IN LISTS DBUS_INCLUDE_DIRS)
    string(APPEND dbus_dirs_i " -I${A}")
    string(APPEND dbus_dirs " ${A}")
endforeach()

target_link_libraries(UntitledDBusUtils PUBLIC ${DBUS_LINK_LIBRARIES})

configure_file(UntitledDBusUtils.pc.in UntitledDBusUtils.pc @ONLY)

if (UIMGUI_INSTALL)
    install(TARGETS UntitledDBusUtils
            LIBRARY DESTINATION lib64/
            ARCHIVE DESTINATION lib64/)
    install(FILES ${UDBUS_HEADERS} DESTINATION include/UntitledDBusUtils)
    install(FILES ${PROJECT_BINARY_DIR}/UntitledDBusUtils.pc DESTINATION lib/pkgconfig/)
endif()
