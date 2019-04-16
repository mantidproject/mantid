set ( SRC_FILES 
  src/IconLoader.cpp
)

set ( INC_FILES 
  inc/MantidQtWidgets/IconLoader.h
)

mtd_add_qt_library (TARGET_NAME MantidQtWidgetsIcons
  QT_VERSION 5
  SRC ${SRC_FILES}
  NOMOC ${INC_FILES}
  INCLUDE_DIRS
    ${CMAKE_CURRENT_SOURCE_DIR}
  LINK_LIBS
    ${TCMALLOC_LIBRARIES_LINKTIME}
    ${POCO_LIBRARIES}
    ${Boost_LIBRARIES}
    ${JSONCPP_LIBRARIES}
  INSTALL_DIR_BASE
    ${PLUGINS_DIR}
  OSX_INSTALL_RPATH
    @loader_path/../../Contents/MacOS
  LINUX_INSTALL_RPATH
    "\$ORIGIN/../../${LIB_DIR}"
)