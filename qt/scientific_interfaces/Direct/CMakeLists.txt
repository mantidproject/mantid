set(SRC_FILES
    ALFView.cpp
    ALFCustomInstrumentView.cpp
    ALFCustomInstrumentPresenter.cpp
    ALFCustomInstrumentModel.cpp)

set(MOC_FILES
    ALFView.h
    ALFCustomInstrumentView.h
    ALFCustomInstrumentPresenter.h)

set(INC_FILES
    DllConfig.h
    ALFCustomInstrumentModel.h
    ALFCustomInstrumentMocks.h)

#Target
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
                     DataObjects
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
add_subdirectory(test)
