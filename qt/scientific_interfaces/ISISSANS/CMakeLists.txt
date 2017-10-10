set ( SRC_FILES
	SANSAddFiles.cpp
	SANSBackgroundCorrectionSettings.cpp
	SANSBackgroundCorrectionWidget.cpp
	SANSConstants.cpp
	SANSDiagnostics.cpp
	SANSEventSlicing.cpp
	SANSPlotSpecial.cpp
	SANSRunWindow.cpp
)

# Include files aren't required, but this makes them appear in Visual Studio
# IMPORTANT: Include files are required in the MOC_FILES set. Scroll down to find it.
set ( INC_FILES
	SANSAddFiles.h
	SANSBackgroundCorrectionSettings.h
	SANSBackgroundCorrectionWidget.h
	SANSConstants.h
	SANSDiagnostics.h
	SANSEventSlicing.h
	SANSPlotSpecial.h
	SANSRunWindow.h
)

set ( MOC_FILES 
    SANSBackgroundCorrectionWidget.h
    SANSAddFiles.h
    SANSPlotSpecial.h
    SANSRunWindow.h
    SANSEventSlicing.h
    SANSDiagnostics.h
)

set ( UI_FILES
    SANSBackgroundCorrectionWidget.ui
    SANSPlotSpecial.ui
    SANSRunWindow.ui
    SANSEventSlicing.ui
)

mtd_add_qt_library (TARGET_NAME MantidScientificInterfacesISISSANS
  SRC ${SRC_FILES}
  MOC ${MOC_FILES}
  NOMOC ${INC_FILES}
  UI ${UI_FILES}
  DEFS IN_MANTIDQT_ISISSANS
  PRECOMPILED PrecompiledHeader.h
  INCLUDE_DIRS
    ${CMAKE_CURRENT_SOURCE_DIR}
  LINK_LIBS
    ${TCMALLOC_LIBRARIES_LINKTIME}
    ${CORE_MANTIDLIBS}
    ${QT_LIBRARIES}
    ${QWT_LIBRARIES}
    ${POCO_LIBRARIES}
    ${Boost_LIBRARIES}
    ${JSONCPP_LIBRARIES}
  MTD_QT_LINK_LIBS
    MantidQtWidgetsCommon
    MantidScientificInterfacesMuon
  INSTALL_DIR ${PLUGINS_DIR}/qtplugins/mantid
  OSX_INSTALL_RPATH "@loader_path/../../../Contents/MacOS"
)

if ( MSVC_IDE )
#  # Add to the 'ScientificInterfaces' group in VS
  set_property ( TARGET MantidScientificInterfacesISISSANSQt4 PROPERTY FOLDER "ScientificInterfaces" )
endif()
