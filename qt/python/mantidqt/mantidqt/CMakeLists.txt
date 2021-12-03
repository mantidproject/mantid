include(SipQtTargetFunctions)

set(COMMON_INC_DIR ${CMAKE_CURRENT_LIST_DIR}/../../../widgets/common/inc)
set(COMMON_SIP_DIR ${CMAKE_CURRENT_LIST_DIR}/../sip)
set(_header_depends
    ${COMMON_SIP_DIR}/vector.sip.h
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
    ${COMMON_INC_DIR}/MantidQtWidgets/Common/InstrumentSelector.h
)

list(APPEND common_link_libs ${CORE_MANTIDLIBS} ${POCO_LIBRARIES} Mantid::PythonInterfaceCore)

# Wrapper module linked against Qt5
mtd_add_sip_module(
  MODULE_NAME _commonqt5
  TARGET_NAME mantidqt_commonqt5
  HEADER_DEPS ${_header_depends}
  SIP_SRC _common.sip
  PYQT_VERSION 5
  INCLUDE_DIRS ${CMAKE_CURRENT_LIST_DIR} ${COMMON_SIP_DIR}
  LINK_LIBS MantidQtWidgetsCommonQt5
            ${common_link_libs}
            Qt5::Core
            Qt5::Widgets
            Qt5::Gui
            Qt5::Qscintilla
            Mantid::API
  INSTALL_DIR ${WORKBENCH_SITE_PACKAGES}/mantidqt
  LINUX_INSTALL_RPATH "\$ORIGIN/.."
  OSX_INSTALL_RPATH "@loader_path/..;@loader_path/../../Frameworks"
  FOLDER Qt5
)

add_subdirectory(icons)
add_subdirectory(widgets/instrumentview)
