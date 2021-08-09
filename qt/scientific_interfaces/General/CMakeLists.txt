set(SRC_FILES
    DataComparison.cpp
    MantidEV.cpp
    MantidEVWorker.cpp
    SampleTransmission.cpp
    StepScan.cpp)

# Include files aren't required, but this makes them appear in Visual Studio
# IMPORTANT: Include files are required in the MOC_FILES set. Scroll down to
# find it.
set(INC_FILES
    DataComparison.h
    DllConfig.h
    MantidEV.h
    MantidEVWorker.h
    SampleTransmission.h
    StepScan.h
    Updateable.h)

set(MOC_FILES
    DataComparison.h
    MantidEV.h
    SampleTransmission.h
    StepScan.h)

set(UI_FILES
    DataComparison.ui
    MantidEV.ui
    SampleTransmission.ui
    StepScan.ui)

mtd_add_qt_library(TARGET_NAME MantidScientificInterfacesGeneral
                   QT_VERSION 5
                   SRC StepScan.cpp
                   MOC StepScan.h
                   NOMOC StepScan.h
                   UI StepScan.ui
                   DEFS IN_MANTIDQT_INTERFACESGENERAL
                   PRECOMPILED PrecompiledHeader.h
                   INCLUDE_DIRS
                     ${CMAKE_CURRENT_SOURCE_DIR}
                   SYSTEM_INCLUDE_DIRS
                     ${Boost_INCLUDE_DIRS}
                   LINK_LIBS
                     ${CORE_MANTIDLIBS}
                     ${POCO_LIBRARIES}
                     ${Boost_LIBRARIES}
                     ${JSONCPP_LIBRARIES}
                     Json
                   MTD_QT_LINK_LIBS
                     MantidQtWidgetsCommon
                     MantidQtWidgetsMplCpp
                   INSTALL_DIR_BASE
                     ${WORKBENCH_PLUGINS_DIR}
                   OSX_INSTALL_RPATH
                     @loader_path/../../Contents/MacOS
                     @loader_path/../../Contents/Frameworks
                   LINUX_INSTALL_RPATH
                     "\$ORIGIN/../../${LIB_DIR}")
