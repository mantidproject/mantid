include ( SipQtTargetFunctions )

set ( COMMON_INC_DIR ../../../widgets/common/inc )
set ( HEADER_DEPENDS
  ${COMMON_INC_DIR}/MantidQtWidgets/Common/AlgorithmDialog.h
  ${COMMON_INC_DIR}/MantidQtWidgets/Common/AlgorithmDialog.h
  ${COMMON_INC_DIR}/MantidQtWidgets/Common/Message.h
  ${COMMON_INC_DIR}/MantidQtWidgets/Common/MessageDisplay.h
)

find_package ( Boost COMPONENTS python REQUIRED )

mtd_add_sip_module (
  MODULE_NAME _widgetscoreqt5
  TARGET_NAME mantidqt_widgetscoreqt5
  SIP_SRCS src/_widgetscore.sip
  HEADER_DEPS ${HEADER_DEPENDS}
  PYQT_VERSION 5
  LINK_LIBS ${TCMALLOC_LIBRARIES_LINKTIME}
    MantidQtWidgetsCommonQt5
    ${POCO_LIBRARIES}
    Qt5::Core
    Qt5::Gui
    Qt5::Widgets
    ${PYTHON_LIBRARIES}
    ${Boost_LIBRARIES}
    API
    Kernel
  FOLDER Qt5
)
