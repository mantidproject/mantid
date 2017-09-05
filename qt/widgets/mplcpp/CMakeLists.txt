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
qt4_wrap_cpp (MOC_GENERATED_SRC ${MOC_HEADERS})

set (NOMOC_HEADERS
  inc/MantidQtWidgets/MplCpp/NDArray1D.h
  inc/MantidQtWidgets/MplCpp/PythonErrors.h
  inc/MantidQtWidgets/MplCpp/PythonObject.h
  inc/MantidQtWidgets/MplCpp/SipUtils.h
)

set (INCLUDES
  ${CMAKE_CURRENT_LIST_DIR}/inc
  ${CMAKE_CURRENT_LIST_DIR}/../commmon/inc
)

# Target
include_directories (SYSTEM ${PYTHON_INCLUDE_PATH} ${PYTHON_NUMPY_INCLUDE_DIR})

add_library (MantidQtWidgetsMplCpp ${LIB_SRCS} ${MOC_GENERATED_SRC} ${MOC_HEADERS} ${NOMOC_HEADERS})
target_include_directories (MantidQtWidgetsMplCpp PUBLIC ${INCLUDES})
target_link_libraries (MantidQtWidgetsMplCpp PRIVATE Qt4::QtGui MantidQtWidgetsCommon ${PYTHON_LIBRARIES})
set_target_properties (MantidQtWidgetsMplCpp PROPERTIES COMPILE_DEFINITIONS IN_MANTIDQT_MPLCPP)
install ( TARGETS MantidQtWidgetsMplCpp ${SYSTEM_PACKAGE_TARGET} DESTINATION ${LIB_DIR} )

# Tests
add_subdirectory (test)
