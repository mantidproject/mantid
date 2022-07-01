# Defines a Python wrapper module for region selector access This wraps the low- level classes.

set(REGIONSELECTOR_INC_DIR ../../widgets/regionselector/inc)
set(_header_depends ${REGIONSELECTOR_INC_DIR}/RegionSelectorObserver.h)

# The wrapper is only defined for Qt5 and does not support legacy Qt4 mode
mtd_add_sip_module(
  MODULE_NAME _regionselectorqt5
  TARGET_NAME mantidqt_regionselectorqt5
  SIP_SRC _regionselector.sip
  PYQT_VERSION 5
  LINK_LIBS Mantid::DataObjects
            ${common_link_libs}
            MantidQtWidgetsRegionSelectorQt5
            MantidQtWidgetsCommonQt5
            MantidQtWidgetsMplCppQt5
            Qt5::Core
            Qt5::Gui
            Qt5::OpenGL
            Qt5::Widgets
  INSTALL_DIR ${WORKBENCH_SITE_PACKAGES}/mantidqt/widgets/regionselector
  LINUX_INSTALL_RPATH "\$ORIGIN/../../.."
  OSX_INSTALL_RPATH "@loader_path/../../.."
  FOLDER Qt5
)
