
# This wraps the low-level classes.

set ( JOBTREEVIEW_INC_DIR ../../../../widgets/common/inc/MantidQtWidgets/Common )
set ( _header_depends
  SIPVector.h
  string.sip
  vector.sip

  ${JOBTREEVIEW_INC_DIR}/Batch/Row.h
  ${JOBTREEVIEW_INC_DIR}/Batch/RowLocation.h
  ${JOBTREEVIEW_INC_DIR}/Batch/JobTreeView.h
  ${JOBTREEVIEW_INC_DIR}/Batch/JobTreeViewSignalAdapter.h
)

list ( APPEND common_link_libs
  ${TCMALLOC_LIBRARIES_LINKTIME}
  ${CORE_MANTIDLIBS}
  ${POCO_LIBRARIES}
  ${PYTHON_LIBRARIES}
)

# Wrapper module linked against Qt4 - not currently installed until required for backward compatability
# with MantidPlot
mtd_add_sip_module (
  MODULE_NAME _jobtreeviewqt4
  TARGET_NAME mantidqt_jobtreeviewqt4
  SIP_SRCS _jobtreeview.sip
  HEADER_DEPS ${_header_depends}
  PYQT_VERSION 4
  INCLUDE_DIRS
    ${CMAKE_CURRENT_LIST_DIR}
    ${PYTHON_INCLUDE_PATH}
    ${CMAKE_SOURCE_DIR}/Framework/PythonInterface/core/inc
  LINK_LIBS
    ${common_link_libs}
    MantidQtWidgetsCommonQt4
    Qt4::QtCore
    Qt4::QtGui
    Qt4::Qscintilla
    ${Boost_LIBRARIES}
    API
  FOLDER Qt4
)

# The wrapper is only defined for Qt5 and does not support
# legacy Qt4 mode
mtd_add_sip_module (
  MODULE_NAME _jobtreeviewqt5
  TARGET_NAME mantidqt_jobtreeviewqt5
  SIP_SRCS _jobtreeview.sip
  HEADER_DEPS ${_header_depends}
  PYQT_VERSION 5
  INCLUDE_DIRS
    ${CMAKE_CURRENT_LIST_DIR}
    ${PYTHON_INCLUDE_PATH}
    ${CMAKE_SOURCE_DIR}/Framework/PythonInterface/core/inc
  LINK_LIBS
    ${common_link_libs}
    MantidQtWidgetsCommonQt5
    MantidQtWidgetsMplCppQt5
    Qt5::Core
    Qt5::Gui
    Qt5::OpenGL
    Qt5::Widgets
  INSTALL_DIR
    ${WORKBENCH_LIB_DIR}/mantidqt/widgets/jobtreeview
  LINUX_INSTALL_RPATH
    "\$ORIGIN/../../.."
  OSX_INSTALL_RPATH
    "@loader_path/../../.."
  FOLDER Qt5
)