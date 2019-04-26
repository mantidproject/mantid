set ( SRC_FILES
  src/CharIconPainter.cpp
  src/CharIconEngine.cpp
  src/Icon.cpp
)

set ( INC_FILES 
  inc/MantidQtIcons/CharIconPainter.h
  inc/MantidQtIcons/CharIconEngine.h
  inc/MantidQtIcons/Icon.h
)

qt5_add_resources(RES_FILES ${CMAKE_CURRENT_LIST_DIR}/resources/icons.qrc)

mtd_add_qt_library (TARGET_NAME MantidQtIcons
  QT_VERSION 5
  SRC ${SRC_FILES}
  NOMOC ${INC_FILES}
  DEFS
    IN_MANTIDQT_ICONS
  INCLUDE_DIRS
    ${CMAKE_CURRENT_SOURCE_DIR}/inc
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

set ( QT5_TEST_FILES
  test/IconTest.h      
)

mtd_add_qt_tests (TARGET_NAME MantidQtIconsTest
  QT_VERSION 5
  SRC ${QT5_TEST_FILES}
  INCLUDE_DIRS
    ${CMAKE_CURRENT_SOURCE_DIR}/inc
  LINK_LIBS
    ${TARGET_LIBRARIES}
    ${Boost_LIBRARIES}
    ${GMOCK_LIBRARIES}
    ${GTEST_LIBRARIES}
  MTD_QT_LINK_LIBS
    MantidQtIcons
  PARENT_DEPENDENCIES
    GUITests
)