set(ICONS_INC_DIR ${CMAKE_CURRENT_LIST_DIR}/../../../icons/inc/)

set(_header_depends ${ICONS_INC_DIR}/MantidQtIcons/Icon.h)

set(WORKBENCH_LINK_LIBS MantidQtIconsQt5 Qt5::Core Qt5::Gui)

if(USE_PYTHON_DYNAMIC_LIB)
	list(APPEND WORKBENCH_LINK_LIBS Python::Python)
endif()

# Wrapper module linked against Qt5
if(ENABLE_WORKBENCH)
  mtd_add_sip_module(
    MODULE_NAME _iconsqt5
    TARGET_NAME mantidqt_iconsqt5
    SIP_SRC _icons.sip
    HEADER_DEPS ${_header_depends}
    PYQT_VERSION 5
    INCLUDE_DIRS ${COMMON_SIP_DIR} ${ICONS_INC_DIR}
                 ${CMAKE_SOURCE_DIR}/Framework/PythonInterface/core/inc
    LINK_LIBS ${WORKBENCH_LINK_LIBS}
    INSTALL_DIR ${WORKBENCH_SITE_PACKAGES}/mantidqt/icons
    LINUX_INSTALL_RPATH "\$ORIGIN/../.."
    OSX_INSTALL_RPATH "@loader_path/../.."
    FOLDER Qt5
  )
endif()

if(APPLE AND NOT USE_PYTHON_DYNAMIC_LIB)
target_link_options(mantidqt_iconsqt5 PRIVATE -Wl,-undefined,dynamic_lookup)
endif()
