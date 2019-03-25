set(SRC_FILES
    DataComparison.cpp
    MantidEV.cpp
    MantidEVWorker.cpp
    SampleTransmission.cpp
    StepScan.cpp
    UserInputValidator.cpp
    background.cpp)

# Include files aren't required, but this makes them appear in Visual Studio
# IMPORTANT: Include files are required in the MOC_FILES set. Scroll down to
# find it.
set(INC_FILES
    Background.h
    DataComparison.h
    DllConfig.h
    MantidEV.h
    MantidEVWorker.h
    SampleTransmission.h
    StepScan.h
    Updateable.h
    UserInputValidator.h)

set(MOC_FILES
    Background.h
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
                   QT_VERSION 4
                   SRC ${SRC_FILES}
                   MOC ${MOC_FILES}
                   NOMOC ${INC_FILES}
                   UI ${UI_FILES}
                   DEFS IN_MANTIDQT_INTERFACESGENERAL
                   PRECOMPILED PrecompiledHeader.h
                   INCLUDE_DIRS
                     ${CMAKE_CURRENT_SOURCE_DIR}
                   LINK_LIBS
                     ${TCMALLOC_LIBRARIES_LINKTIME}
                     ${CORE_MANTIDLIBS}
                     ${POCO_LIBRARIES}
                     ${Boost_LIBRARIES}
                     ${JSONCPP_LIBRARIES}
                   QT4_LINK_LIBS
                     Qwt5
                   MTD_QT_LINK_LIBS
                     MantidQtWidgetsCommon
                     MantidQtWidgetsLegacyQwt
                   INSTALL_DIR_BASE
                     ${PLUGINS_DIR}
                   OSX_INSTALL_RPATH
                     @loader_path/../../Contents/MacOS
                   LINUX_INSTALL_RPATH
                     "\$ORIGIN/../../${LIB_DIR}")
