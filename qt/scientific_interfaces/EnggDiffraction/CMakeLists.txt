set ( SRC_FILES
	EnggDiffFittingPresenter.cpp
	EnggDiffFittingViewQtWidget.cpp
	EnggDiffractionPresenter.cpp
	EnggDiffractionViewQtGUI.cpp
)

# Include files aren't required, but this makes them appear in Visual Studio
# IMPORTANT: Include files are required in the MOC_FILES set. Scroll down to find it.
set ( INC_FILES
	EnggDiffCalibSettings.h
	EnggDiffFittingPresWorker.h
	EnggDiffFittingPresenter.h
	EnggDiffFittingViewQtWidget.h
	EnggDiffractionPresWorker.h
	EnggDiffractionPresenter.h
	EnggDiffractionViewQtGUI.h
	IEnggDiffFittingPresenter.h
	IEnggDiffractionPresenter.h
	IEnggDiffractionView.h
)

set ( MOC_FILES 
    EnggDiffFittingPresenter.h
    EnggDiffFittingPresWorker.h
    EnggDiffFittingViewQtWidget.h
    EnggDiffractionPresenter.h
    EnggDiffractionPresWorker.h
    EnggDiffractionViewQtGUI.h
)

set ( UI_FILES 
   EnggDiffractionQtGUI.ui
   EnggDiffractionQtTabCalib.ui
   EnggDiffractionQtTabFocus.ui
   EnggDiffractionQtTabPreproc.ui
   EnggDiffractionQtTabFitting.ui
   EnggDiffractionQtTabSettings.ui
)

qt4_wrap_cpp ( MOCCED_FILES ${MOC_FILES} OPTIONS ${extra_options} )

set ( ALL_SRC ${SRC_FILES} ${MOCCED_FILES} )

qt4_wrap_ui ( UI_HDRS ${UI_FILES} )
include_directories ( ${CMAKE_CURRENT_BINARY_DIR} )

# Use a precompiled header where they are supported
enable_precompiled_headers( PrecompiledHeader.h  ALL_SRC )
add_library ( MantidScientificInterfacesEnggDiffraction ${ALL_SRC} ${INC_FILES} ${UI_HDRS} )

# Set the name of the generated library
set_target_properties ( MantidScientificInterfacesEnggDiffraction PROPERTIES COMPILE_DEFINITIONS IN_MANTIDQT_ENGGDIFFRACTION )
if ( MSVC_IDE )
#  # Add to the 'ScientificInterfaces' group in VS
  set_property ( TARGET MantidScientificInterfacesEnggDiffraction PROPERTY FOLDER "ScientificInterfaces" )
endif()

if (OSX_VERSION VERSION_GREATER 10.8)
  set_target_properties ( MantidScientificInterfacesEnggDiffraction PROPERTIES INSTALL_RPATH "@loader_path/../../../Contents/MacOS")
endif ()

find_Package(Threads)

target_link_libraries ( MantidScientificInterfacesEnggDiffraction LINK_PRIVATE ${TCMALLOC_LIBRARIES_LINKTIME}
            MantidQtWidgetsCommonQt4
            MantidScientificInterfacesMuon
            ${CORE_MANTIDLIBS}
            ${QT_LIBRARIES}
            ${QWT_LIBRARIES}
            ${POCO_LIBRARIES}
            ${Boost_LIBRARIES}
            ${JSONCPP_LIBRARIES}
            ${CMAKE_THREAD_LIBS_INIT})

###########################################################################
# Installation settings
###########################################################################

install ( TARGETS MantidScientificInterfacesEnggDiffraction ${SYSTEM_PACKAGE_TARGET} DESTINATION ${PLUGINS_DIR}/qtplugins/mantid )
