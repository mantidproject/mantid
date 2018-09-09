# Rules for matplotlib cpp library
set ( LIB_SRCS
  src/Artist.cpp
  src/Axes.cpp
  src/FigureCanvasQt.cpp
)

set ( MOC_HEADERS
  inc/MantidQtWidgets/MplCpp/FigureCanvasQt.h
)

set (NOMOC_HEADERS
  inc/MantidQtWidgets/MplCpp/Artist.h
  inc/MantidQtWidgets/MplCpp/Axes.h
)

find_package ( BoostPython REQUIRED )

# Target
mtd_add_qt_library (TARGET_NAME MplCpp
  OUTPUT_NAME MantidQtWidgetsMplCpp
  QT_VERSION 5
  SRC ${LIB_SRCS}
  MOC ${MOC_HEADERS}
  NOMOC ${NOMOC_HEADERS}
  DEFS
    IN_MANTIDQT_MPLCPP
  INCLUDE_DIRS
      inc
      ../../../Framework/PythonInterface/inc
      ${PYTHON_INCLUDE_PATH}
      ${PYTHON_NUMPY_INCLUDE_DIR}
  LINK_LIBS
    ${TCMALLOC_LIBRARIES_LINKTIME}
    ${Boost_LIBRARIES}
    ${PYTHON_LIBRARIES}
    PythonInterfaceCore
  INSTALL_DIR
    ${LIB_DIR}
  OSX_INSTALL_RPATH
    @loader_path/../MacOS
    @loader_path/../Libraries
  LINUX_INSTALL_RPATH
    "\$ORIGIN/../${LIB_DIR}"
)

# Testing
add_subdirectory ( test )
