# Defines a Python wrapper module for instrument view access This wraps the low- level classes.

set(INSTRUMENTVIEW_INC_DIR ../../widgets/instrumentview/inc)
set(_header_depends ${INSTRUMENTVIEW_INC_DIR}/InstrumentWidget.h)

# The wrapper is only defined for Qt5
mtd_add_sip_module(
  MODULE_NAME _instrumentviewqt5
  TARGET_NAME mantidqt_instrumentviewqt5
  SIP_SRC _instrumentview.sip
  PYQT_VERSION 5
  LINK_LIBS Mantid::DataObjects
            ${common_link_libs}
            MantidQtWidgetsInstrumentViewQt5
            MantidQtWidgetsCommonQt5
            MantidQtWidgetsMplCppQt5
            Qt5::Core
            Qt5::Gui
            Qt5::OpenGL
            Qt5::Widgets
  INSTALL_DIR ${Python_SITELIB_RELPATH}/mantidqt/widgets/instrumentview
  LINUX_INSTALL_RPATH "\$ORIGIN/../../.."
  OSX_INSTALL_RPATH "@loader_path/../../.."
  FOLDER Qt5
)
