set(ICONS_INC_DIR ${CMAKE_CURRENT_LIST_DIR}/../../../../icons/inc/)

set(_header_depends ${ICONS_INC_DIR}/MantidQtIcons/Icon.h)

set(WORKBENCH_LINK_LIBS MantidQtIconsQt${MANTID_QT_VERSION} Qt${MANTID_QT_VERSION}::Core Qt${MANTID_QT_VERSION}::Gui)

if(USE_PYTHON_DYNAMIC_LIB)
  list(APPEND WORKBENCH_LINK_LIBS Python::Python)
endif()

# Wrapper module linked against Qt${MANTID_QT_VERSION}
mtd_add_sip_module(
  MODULE_NAME _iconsqt${MANTID_QT_VERSION}
  TARGET_NAME mantidqt_iconsqt${MANTID_QT_VERSION}
  SIP_SRC _icons.sip.in
  HEADER_DEPS ${_header_depends}
  PYQT_VERSION ${MANTID_QT_VERSION}
  INCLUDE_DIRS ${COMMON_SIP_DIR} ${ICONS_INC_DIR} ${CMAKE_SOURCE_DIR}/Framework/PythonInterface/core/inc
  LINK_LIBS ${WORKBENCH_LINK_LIBS}
  INSTALL_DIR ${Python_SITELIB_RELPATH}/mantidqt/icons
  LINUX_INSTALL_RPATH "\$ORIGIN/../.."
  OSX_INSTALL_RPATH "@loader_path/../..;@loader_path/../../Frameworks"
  FOLDER Qt${MANTID_QT_VERSION}
)

if(APPLE AND NOT USE_PYTHON_DYNAMIC_LIB)
  target_link_options(mantidqt_iconsqt${MANTID_QT_VERSION} PRIVATE -Wl,-undefined,dynamic_lookup)
endif()
