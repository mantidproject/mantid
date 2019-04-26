set(ICONS_INC_DIR ${CMAKE_CURRENT_LIST_DIR}/../../../icons/inc/)

set(
  _header_depends
  ${COMMON_SIP_DIR}/SIPVector.h
  ${COMMON_SIP_DIR}/vector.sip
  ${ICONS_INC_DIR}/MantidQtIcons/Icon.h
)



# Wrapper module linked against Qt5
if(ENABLE_WORKBENCH)
     mtd_add_sip_module(MODULE_NAME _iconsqt5
                     QT_VERSION 5
                     TARGET_NAME mantidqt_iconsqt5
                     SIP_SRCS _icons.sip
                     HEADER_DEPS
                       ${_header_depends}
                     PYQT_VERSION 5
                     INCLUDE_DIRS
                       ${COMMON_SIP_DIR}
                       ${PYTHON_INCLUDE_PATH}
                       ${ICONS_INC_DIR}
                       ${CMAKE_SOURCE_DIR}/Framework/PythonInterface/core/inc
                     LINK_LIBS
                       MantidQtIconsQt5
                       Qt5::Core
                       Qt5::Gui
                     INSTALL_DIR
                       ${WORKBENCH_LIB_DIR}/mantidqt
                     LINUX_INSTALL_RPATH
                       "\$ORIGIN/.."
                     OSX_INSTALL_RPATH
                       "@loader_path/.."
                     FOLDER Qt5)
endif()