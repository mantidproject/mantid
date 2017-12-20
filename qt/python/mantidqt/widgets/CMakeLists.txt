include ( SipQtTargetFunctions )

set ( COMMON_INC_DIR ../../../widgets/common/inc )
set ( HEADER_DEPENDS
  ${COMMON_INC_DIR}/MantidQtWidgets/Common/Message.h
  ${COMMON_INC_DIR}/MantidQtWidgets/Common/MessageDisplay.h
)

list ( APPEND common_link_libs
  ${TCMALLOC_LIBRARIES_LINKTIME}
  Kernel
  ${POCO_LIBRARIES}
  ${PYTHON_LIBRARIES}
)

# Wrapper module linked against Qt4
mtd_add_sip_module (
  MODULE_NAME _widgetscoreqt4
  TARGET_NAME mantidqt_widgetscoreqt4
  SIP_SRCS src/_widgetscore.sip
  HEADER_DEPS ${HEADER_DEPENDS}
  PYQT_VERSION 4
  LINK_LIBS
    ${common_link_libs}
    MantidQtWidgetsCommonQt4
    Qt4::QtCore
    Qt4::QtGui
    ${PYTHON_LIBRARIES}
  FOLDER Qt4
)

# Wrapper module linked against Qt5
mtd_add_sip_module (
  MODULE_NAME _widgetscoreqt5
  TARGET_NAME mantidqt_widgetscoreqt5
  SIP_SRCS src/_widgetscore.sip
  HEADER_DEPS ${HEADER_DEPENDS}
  PYQT_VERSION 5
  LINK_LIBS
    ${common_link_libs}
    MantidQtWidgetsCommonQt5
    Qt5::Core
    Qt5::Gui
    Qt5::Widgets
  FOLDER Qt5
)
