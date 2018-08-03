include ( SipQtTargetFunctions )

set ( COMMON_INC_DIR ../../../widgets/common/inc )
set ( HEADER_DEPENDS
  ${COMMON_INC_DIR}/MantidQtWidgets/Common/AlgorithmDialog.h
  ${COMMON_INC_DIR}/MantidQtWidgets/Common/Message.h
  ${COMMON_INC_DIR}/MantidQtWidgets/Common/MessageDisplay.h
  ${COMMON_INC_DIR}/MantidQtWidgets/Common/WorkspacePresenter/WorkspaceTreeWidget.h
)

find_package ( BoostPython REQUIRED )

list ( APPEND common_link_libs
  ${TCMALLOC_LIBRARIES_LINKTIME}
  ${CORE_MANTIDLIBS}
  ${POCO_LIBRARIES}
  ${PYTHON_LIBRARIES}
)

# Wrapper module linked against Qt4
mtd_add_sip_module (
  MODULE_NAME _commonqt4
  TARGET_NAME mantidqt_commonqt4
  SIP_SRCS _common.sip
  HEADER_DEPS ${HEADER_DEPENDS}
  PYQT_VERSION 4
  LINK_LIBS
    ${common_link_libs}
    MantidQtWidgetsCommonQt4
    Qt4::QtCore
    Qt4::QtGui
    Qt4::Qscintilla
    ${PYTHON_LIBRARIES}
    ${Boost_LIBRARIES}
    API
  INSTALL_DIR
    ${LIB_DIR}/mantidqt
  FOLDER Qt4
)

if ( ENABLE_WORKBENCH )
  find_package ( QScintillaQt5 REQUIRED )
endif()

# Wrapper module linked against Qt5
mtd_add_sip_module (
  MODULE_NAME _commonqt5
  TARGET_NAME mantidqt_commonqt5
  SIP_SRCS _common.sip
  HEADER_DEPS ${HEADER_DEPENDS}
  PYQT_VERSION 5
  LINK_LIBS
    ${common_link_libs}
    MantidQtWidgetsCommonQt5
    ${POCO_LIBRARIES}
    Qt5::Core
    Qt5::Gui
    Qt5::Widgets
    Qt5::Qscintilla
    ${PYTHON_LIBRARIES}
    ${Boost_LIBRARIES}
    API
  INSTALL_DIR
    ${LIB_DIR}/mantidqt
  FOLDER Qt5
)
