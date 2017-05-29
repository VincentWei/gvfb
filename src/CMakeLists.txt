SET (PROJECT_NAME gvfb)

LIST (APPEND SRC_LIST
    main.c gvfb_main.c gvfb_view.c gvfb_input.c gvfb_callbacks.c
    gvfb_interface.c gvfb_errmsg.c deviceskin.c skin.c)

IF (WIN32)
    LIST (APPEND SRC_LIST gvfb_win32.c getopt_long.c)
ELSE ()
    LIST (APPEND SRC_LIST gvfb_linux.c) 
ENDIF (WIN32)

#pkg-config info
INCLUDE (FindPkgConfig)

PKG_CHECK_MODULES (GTK gtk+-2.0 gthread-2.0)

IF (NOT GTK_FOUND)
    MESSAGE(FATAL_ERROR "Gtk+-2.0 not found")
ENDIF (NOT GTK_FOUND)

INCLUDE_DIRECTORIES (${GTK_INCLUDE_DIRS})
LINK_DIRECTORIES (${CMAKE_LIBRARY_PATH} ${GTK_LIBRARY_DIRS})
LIST (APPEND linkflags ${GTK_LIBRARIES})

ADD_EXECUTABLE (${PROJECT_NAME} ${SRC_LIST})

TARGET_LINK_LIBRARIES (${PROJECT_NAME} ${linkflags})
INSTALL (TARGETS ${PROJECT_NAME} RUNTIME DESTINATION bin)

