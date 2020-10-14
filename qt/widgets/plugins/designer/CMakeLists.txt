set(SRC_FILES src/DesignerPlugin.cpp src/PluginCollectionInterface.cpp)

# Include files aren't required, but this makes them appear in Visual Studio
set(INC_FILES
    inc/MantidQtWidgets/Plugins/Designer/DesignerPlugin.h
    inc/MantidQtWidgets/Plugins/Designer/PluginCollectionInterface.h)

set(MOC_FILES ${INC_FILES})

# Moc needs the include directores for the designer before running
include_directories(SYSTEM ${QT_QTDESIGNER_INCLUDE_DIR})

mtd_add_qt_library(TARGET_NAME MantidQtWidgetsPluginsDesigner
                   QT_VERSION 4
                   SRC ${SRC_FILES}
                   MOC ${MOC_FILES}
                   NOMOC ${INC_FILES}
                   DEFS
                     QSCINTILLA_DLL
                     LIBRARY_NAME=MantidQtWidgetsPluginsDesigner
                   INCLUDE_DIRS
                     inc
                     ../../common
                     ../../plotting
                   LINK_LIBS
                     ${CORE_MANTIDLIBS}
                     ${POCO_LIBRARIES}
                     ${Boost_LIBRARIES}
                   QT4_LINK_LIBS
                     Qt4::Qscintilla
                     Qwt5
                   MTD_QT_LINK_LIBS
                     MantidQtWidgetsCommon
                     MantidQtWidgetsPlotting
                     MantidQtWidgetsSliceViewer
                   OUTPUT_SUBDIR
                     designer
                   EXCLUDE_FROM_ALL)
