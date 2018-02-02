# Rules for matplotlib cpp library
set (LIB_SRCS
  src/MplFigureCanvas.cpp
  src/NDArray1D.cpp
  src/PythonErrors.cpp
  src/PythonObject.cpp
  src/SipUtils.cpp
)

set (MOC_HEADERS
  inc/MantidQtWidgets/MplCpp/MplFigureCanvas.h
)
#qt4_wrap_cpp (MOC_GENERATED_SRC ${MOC_HEADERS})

set (NOMOC_HEADERS
  inc/MantidQtWidgets/MplCpp/NDArray1D.h
  inc/MantidQtWidgets/MplCpp/PythonErrors.h
  inc/MantidQtWidgets/MplCpp/PythonObject.h
  inc/MantidQtWidgets/MplCpp/SipUtils.h
)

#set (INCLUDES
#  ${CMAKE_CURRENT_LIST_DIR}/inc
#  ${CMAKE_CURRENT_LIST_DIR}/../common/inc
#)

## Target
#include_directories (SYSTEM ${PYTHON_INCLUDE_PATH} ${PYTHON_NUMPY_INCLUDE_DIR})

#add_library (MantidQtWidgetsMplCpp ${LIB_SRCS} ${MOC_GENERATED_SRC} ${MOC_HEADERS} ${NOMOC_HEADERS})
#target_include_directories (MantidQtWidgetsMplCpp PUBLIC ${INCLUDES} ${PYTHON_INCLUDE_PATH})
#target_link_libraries (MantidQtWidgetsMplCpp PRIVATE Qt4::QtGui MantidQtWidgetsCommon ${PYTHON_LIBRARIES})
#set_target_properties (MantidQtWidgetsMplCpp PROPERTIES COMPILE_DEFINITIONS IN_MANTIDQT_MPLCPP)
#install ( TARGETS MantidQtWidgetsMplCpp ${SYSTEM_PACKAGE_TARGET} DESTINATION ${LIB_DIR} )

## Tests
#add_subdirectory (test)

# Target
mtd_add_qt_library (TARGET_NAME MantidQtWidgetsMplCpp
  SRC ${LIB_SRCS}
  MOC ${MOC_HEADERS}
  NOMOC ${NOMOC_HEADERS}
  DEFS
    IN_MANTIDQT_MPLCPP
  INCLUDE_DIRS
      inc
      ../common/inc
      ${PYTHON_INCLUDE_PATH}
      ${PYTHON_NUMPY_INCLUDE_DIR}
  LINK_LIBS
    ${TCMALLOC_LIBRARIES_LINKTIME}
    ${CORE_MANTIDLIBS}
    ${POCO_LIBRARIES}
    ${Boost_LIBRARIES}
    ${PYTHON_LIBRARIES}
    ${OPENGL_LIBRARIES}
  MTD_QT_LINK_LIBS
    MantidQtWidgetsCommon
)
