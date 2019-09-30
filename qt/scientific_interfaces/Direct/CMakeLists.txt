set(SRC_FILES
    ALFView.cpp
    ALFView_view.cpp
    ALFView_presenter.cpp
    ALFView_model.cpp)

set(MOC_FILES
    ALFView.h
    ALFView_view.h
    ALFView_presenter.h)

set(INC_FILES
    DllConfig.h)

# Target
mtd_add_qt_library(TARGET_NAME MantidScientificInterfacesDirect
                   QT_VERSION 4
                   SRC
                     ${SRC_FILES}
                   MOC
                     ${MOC_FILES}
                   NOMOC
                     ${INC_FILES}
                   DEFS
                     IN_MANTIDQT_DIRECT
                   SYSTEM_INCLUDE_DIRS
                     ${Boost_INCLUDE_DIRS}
                   LINK_LIBS
                     ${TCMALLOC_LIBRARIES_LINKTIME}
                     ${CORE_MANTIDLIBS}
                     ${POCO_LIBRARIES}
                     ${Boost_LIBRARIES}
                     ${PYTHON_LIBRARIES}
                     ${OPENGL_gl_LIBRARY}
                     ${OPENGL_glu_LIBRARY}
                   QT4_LINK_LIBS
                     Qt4::QtOpenGL
                     Qwt5
                   MTD_QT_LINK_LIBS
                     MantidQtWidgetsCommon
                     MantidQtWidgetsPlotting
                     MantidQtWidgetsInstrumentView
                   INSTALL_DIR_BASE
                     ${PLUGINS_DIR}
                   OSX_INSTALL_RPATH
                     @loader_path/../../Contents/MacOS
                     @loader_path/../../plugins/qt4
                   LINUX_INSTALL_RPATH
                     "\$ORIGIN/../../${LIB_DIR};\$ORIGIN/../../plugins/qt4/")

find_package(BoostPython REQUIRED)
mtd_add_qt_library(TARGET_NAME MantidScientificInterfacesDirect
                   QT_VERSION 5
                   SRC
                     ${SRC_FILES}
                   MOC
                     ${MOC_FILES}
                   NOMOC
                     ${INC_FILES}
                   DEFS
                     IN_MANTIDQT_DIRECT
                   LINK_LIBS
                     ${TCMALLOC_LIBRARIES_LINKTIME}
                     ${CORE_MANTIDLIBS}
                     PythonInterfaceCore
                     ${POCO_LIBRARIES}
                     ${Boost_LIBRARIES}
                     ${PYTHON_LIBRARIES}
                     ${OPENGL_gl_LIBRARY}
                     ${OPENGL_glu_LIBRARY}
                   QT5_LINK_LIBS
                     Qt5::OpenGL
                   MTD_QT_LINK_LIBS
                     MantidQtWidgetsCommon
                     MantidQtWidgetsInstrumentView
					 MantidQtWidgetsPlotting
                     MantidQtWidgetsMplCpp
                   INSTALL_DIR_BASE
                     ${WORKBENCH_PLUGINS_DIR}
                   OSX_INSTALL_RPATH
                     @loader_path/../../Contents/MacOS
                     @loader_path/../../plugins/qt5
                   LINUX_INSTALL_RPATH
                     "\$ORIGIN/../../${LIB_DIR};\$ORIGIN/../../plugins/qt5/") 
# Testing target
# add_subdirectory(test)
