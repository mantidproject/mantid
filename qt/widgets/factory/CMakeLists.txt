set(SRC_FILES
    src/WidgetFactory.cpp)

set(INC_FILES
    inc/MantidQtWidgets/Factory/WidgetFactory.h)

set(MOC_FILES
    inc/MantidQtWidgets/Factory/WidgetFactory.h)

set(UI_FILES)

set(TEST_FILES
    test/WidgetFactoryTest.h)

# Target
mtd_add_qt_library(TARGET_NAME MantidQtWidgetsFactory
                   QT_VERSION 4
                   SRC ${SRC_FILES}
                   MOC ${MOC_FILES}
                   NOMOC ${INC_FILES}
                   DEFS
                     IN_MANTIDQT_FACTORY
                   INCLUDE_DIRS
                     inc
                   SYSTEM_INCLUDE_DIRS
                     ${Boost_INCLUDE_DIRS}
                   LINK_LIBS
                     ${CORE_MANTIDLIBS}
                     ${POCO_LIBRARIES}
                     ${Boost_LIBRARIES}
                   QT4_LINK_LIBS
                     Qwt5
                   MTD_QT_LINK_LIBS
                     MantidQtWidgetsCommon
                     MantidQtWidgetsPlotting
                     MantidQtWidgetsSliceViewer
                   INSTALL_DIR
                     ${LIB_DIR}
                   LINUX_INSTALL_RPATH
                     "\$ORIGIN/../${LIB_DIR}")
