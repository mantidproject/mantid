# Defines a Python wrapper module for instrument view access This wraps the low- level classes.

set(INSTRUMENTVIEW_INC_DIR ../../widgets/instrumentview/inc)
set(_header_depends ${INSTRUMENTVIEW_INC_DIR}/InstrumentWidget.h)

# QOpenGLWidget lives in the OpenGL module on Qt5 but in its own OpenGLWidgets module on Qt6
set(_instrumentview_qt_libs Qt${MANTID_QT_VERSION}::Core Qt${MANTID_QT_VERSION}::Gui Qt${MANTID_QT_VERSION}::OpenGL
                            Qt${MANTID_QT_VERSION}::Widgets
)
if(MANTID_QT_VERSION EQUAL 6)
  list(APPEND _instrumentview_qt_libs Qt6::OpenGLWidgets)
endif()

# The wrapper is only defined for Qt${MANTID_QT_VERSION}
mtd_add_sip_module(
  MODULE_NAME _instrumentviewqt${MANTID_QT_VERSION}
  TARGET_NAME mantidqt_instrumentviewqt${MANTID_QT_VERSION}
  SIP_SRC _instrumentview.sip.in
  PYQT_VERSION ${MANTID_QT_VERSION}
  LINK_LIBS Mantid::DataObjects ${common_link_libs} MantidQtWidgetsInstrumentViewQt${MANTID_QT_VERSION}
            MantidQtWidgetsCommonQt${MANTID_QT_VERSION} MantidQtWidgetsMplCppQt${MANTID_QT_VERSION}
            ${_instrumentview_qt_libs}
  INSTALL_DIR ${Python_SITELIB_RELPATH}/mantidqt/widgets/instrumentview
  LINUX_INSTALL_RPATH "\$ORIGIN/../../.."
  OSX_INSTALL_RPATH "@loader_path/../../.."
  FOLDER Qt${MANTID_QT_VERSION}
)
