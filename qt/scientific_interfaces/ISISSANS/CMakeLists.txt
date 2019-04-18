set(SRC_FILES
    SANSAddFiles.cpp
    SANSBackgroundCorrectionSettings.cpp
    SANSBackgroundCorrectionWidget.cpp
    SANSConstants.cpp
    SANSDiagnostics.cpp
    SANSEventSlicing.cpp
    SANSPlotSpecial.cpp
    SANSRunWindow.cpp)

# Include files aren't required, but this makes them appear in Visual Studio
# IMPORTANT: Include files are required in the MOC_FILES set. Scroll down to
# find it.
set(INC_FILES
    SANSAddFiles.h
    SANSBackgroundCorrectionSettings.h
    SANSBackgroundCorrectionWidget.h
    SANSConstants.h
    SANSDiagnostics.h
    SANSEventSlicing.h
    SANSPlotSpecial.h
    SANSRunWindow.h)

set(MOC_FILES
    SANSBackgroundCorrectionWidget.h
    SANSAddFiles.h
    SANSPlotSpecial.h
    SANSRunWindow.h
    SANSEventSlicing.h
    SANSDiagnostics.h)

set(UI_FILES
    SANSBackgroundCorrectionWidget.ui
    SANSPlotSpecial.ui
    SANSRunWindow.ui
    SANSEventSlicing.ui)

mtd_add_qt_library(TARGET_NAME MantidScientificInterfacesISISSANS
                   QT_VERSION 4
                   SRC ${SRC_FILES}
                   MOC ${MOC_FILES}
                   NOMOC ${INC_FILES}
                   UI ${UI_FILES}
                   DEFS IN_MANTIDQT_ISISSANS
                   PRECOMPILED PrecompiledHeader.h
                   INCLUDE_DIRS
                     ${CMAKE_CURRENT_SOURCE_DIR}
                   SYSTEM_INCLUDE_DIRS
                     ${Boost_INCLUDE_DIRS}
                   LINK_LIBS
                     ${TCMALLOC_LIBRARIES_LINKTIME}
                     ${CORE_MANTIDLIBS}
                     ${QT_LIBRARIES}
                     ${POCO_LIBRARIES}
                     ${Boost_LIBRARIES}
                     ${JSONCPP_LIBRARIES}
                   QT4_LINK_LIBS
                     Qwt5
                   MTD_QT_LINK_LIBS
                     MantidQtWidgetsCommon
                     MantidQtWidgetsPlotting
                     MantidScientificInterfacesMuon
                   INSTALL_DIR_BASE
                     ${PLUGINS_DIR}
                   OSX_INSTALL_RPATH
                     @loader_path/../../Contents/MacOS
                     @loader_path/../../plugins/qt4
                   LINUX_INSTALL_RPATH
                     "\$ORIGIN/../../${LIB_DIR};\$ORIGIN/../../plugins/qt4/")
