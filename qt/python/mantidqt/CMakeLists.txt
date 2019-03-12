include ( SipQtTargetFunctions )

set ( COMMON_INC_DIR ../../widgets/common/inc )
set ( COMMON_SIP_DIR ../common_sip )
set ( _header_depends
  ${COMMON_SIP_DIR}/SIPVector.h
  ${COMMON_SIP_DIR}/string.sip
  ${COMMON_SIP_DIR}/vector.sip

  ${COMMON_INC_DIR}/MantidQtWidgets/Common/AlgorithmDialog.h
  ${COMMON_INC_DIR}/MantidQtWidgets/Common/Message.h
  ${COMMON_INC_DIR}/MantidQtWidgets/Common/MessageDisplay.h
  ${COMMON_INC_DIR}/MantidQtWidgets/Common/WorkspacePresenter/WorkspaceTreeWidget.h
  ${COMMON_INC_DIR}/MantidQtWidgets/Common/HintStrategy.h
  ${COMMON_INC_DIR}/MantidQtWidgets/Common/Hint.h

  ${COMMON_INC_DIR}/MantidQtWidgets/Common/Batch/Row.h
  ${COMMON_INC_DIR}/MantidQtWidgets/Common/Batch/RowLocation.h
  ${COMMON_INC_DIR}/MantidQtWidgets/Common/Batch/JobTreeView.h
  ${COMMON_INC_DIR}/MantidQtWidgets/Common/Batch/JobTreeViewSignalAdapter.h
)

find_package ( BoostPython REQUIRED )

list ( APPEND common_link_libs
  ${TCMALLOC_LIBRARIES_LINKTIME}
  ${CORE_MANTIDLIBS}
  ${POCO_LIBRARIES}
  ${PYTHON_LIBRARIES}
)

# Wrapper module linked against Qt4
if ( ENABLE_MANTIDPLOT )
  mtd_add_sip_module (
    MODULE_NAME _commonqt4
    TARGET_NAME mantidqt_commonqt4
    SIP_SRCS _common.sip
    HEADER_DEPS ${_header_depends}
    PYQT_VERSION 4
    INCLUDE_DIRS
      ${CMAKE_CURRENT_LIST_DIR}
      ${COMMON_SIP_DIR}
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
    INSTALL_DIR
      ${LIB_DIR}/mantidqt
    LINUX_INSTALL_RPATH
      "\$ORIGIN/.."
    OSX_INSTALL_RPATH
      "@loader_path/.."
    FOLDER Qt4
  )
endif()

# Wrapper module linked against Qt5
if ( ENABLE_WORKBENCH )
  find_package ( QScintillaQt5 REQUIRED )

  mtd_add_sip_module (
    MODULE_NAME _commonqt5
    TARGET_NAME mantidqt_commonqt5
    SIP_SRCS _common.sip
    HEADER_DEPS ${_header_depends}
    PYQT_VERSION 5
    INCLUDE_DIRS
      ${CMAKE_CURRENT_LIST_DIR}
      ${COMMON_SIP_DIR}
      ${PYTHON_INCLUDE_PATH}
      ${CMAKE_SOURCE_DIR}/Framework/PythonInterface/core/inc
    LINK_LIBS
      ${common_link_libs}
      MantidQtWidgetsCommonQt5
      Qt5::Core
      Qt5::Gui
      Qt5::Widgets
      Qt5::Qscintilla
      ${Boost_LIBRARIES}
      API
    INSTALL_DIR
      ${WORKBENCH_LIB_DIR}/mantidqt
    LINUX_INSTALL_RPATH
      "\$ORIGIN/.."
    OSX_INSTALL_RPATH
      "@loader_path/.." 
    FOLDER Qt5
  )
endif()

add_subdirectory ( widgets/instrumentview )
