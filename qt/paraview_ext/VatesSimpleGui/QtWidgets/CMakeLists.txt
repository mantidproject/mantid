project(MantidVatesSimpleGuiQtWidgets)

# These are the C++ files to be compiled.
set(INCLUDE_FILES
    inc/MantidVatesSimpleGuiQtWidgets/AxisInformation.h
    inc/MantidVatesSimpleGuiQtWidgets/GeometryParser.h
    inc/MantidVatesSimpleGuiQtWidgets/ModeControlWidget.h
    inc/MantidVatesSimpleGuiQtWidgets/RotationPointDialog.h
    inc/MantidVatesSimpleGuiQtWidgets/WidgetDllOption.h)

set(SOURCE_FILES
    src/AxisInformation.cpp
    src/GeometryParser.cpp
    src/ModeControlWidget.cpp
    src/RotationPointDialog.cpp)

set(MOC_FILES inc/MantidVatesSimpleGuiQtWidgets/ModeControlWidget.h
    inc/MantidVatesSimpleGuiQtWidgets/RotationPointDialog.h)

set(RES_FILES icons/QtWidgetsIcons.qrc)

set(UI_FILES inc/MantidVatesSimpleGuiQtWidgets/ModeControlWidget.ui
    inc/MantidVatesSimpleGuiQtWidgets/RotationPointDialog.ui)

mtd_add_qt_library(
  TARGET_NAME
  VatesSimpleGuiQtWidgets
  QT_VERSION
  4
  SRC
  ${SOURCE_FILES}
  MOC
  ${MOC_FILES}
  NOMOC
  ${INCLUDE_FILES}
  UI
  ${UI_FILES}
  RES
  ${RES_FILES}
  DEFS
  IN_MANTID_VATES_SIMPLEGUI_QTWIDGETS
  INCLUDE_DIRS
  inc
  LINK_LIBS
  ${TCMALLOC_LIBRARIES_LINKTIME}
  ${POCO_LIBRARIES}
  ${MANTID_SUBPROJECT_LIBS}
  QT4_LINK_LIBS
  Qwt5
  MTD_QT_LINK_LIBS
  MantidQtWidgetsCommon
  INSTALL_DIR_BASE
  ${PLUGINS_DIR}
  OSX_INSTALL_RPATH
  @loader_path/../../Contents/Libraries
  @loader_path/../../Contents/MacOS
  LINUX_INSTALL_RPATH
  "\$ORIGIN/../../${LIB_DIR};\$ORIGIN/../../${LIB_DIR}/paraview-${ParaView_VERSION_MAJOR}.${ParaView_VERSION_MINOR}"
  )

# Set the name of the generated library
set_target_properties(VatesSimpleGuiQtWidgetsQt4
                      PROPERTIES OUTPUT_NAME MantidVatesSimpleGuiQtWidgetsQt4)
# Add to the 'VatesSimpleGui' group in VS
set_property(TARGET VatesSimpleGuiQtWidgetsQt4
             PROPERTY FOLDER MantidVatesSimpleGui)
