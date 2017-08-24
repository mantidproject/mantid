set ( SRC_FILES
	MDFAddWorkspaceDialog.cpp
	MDFDataController.cpp
	MDFDatasetPlotData.cpp
	MDFEditLocalParameterDialog.cpp
	MDFFunctionPlotData.cpp
	MDFLocalParameterEditor.cpp
	MDFLocalParameterItemDelegate.cpp
	MDFLogValueFinder.cpp
	MDFPlotController.cpp
	MultiDatasetFit.cpp
)

# Include files aren't required, but this makes them appear in Visual Studio
# IMPORTANT: Include files are required in the MOC_FILES set. Scroll down to find it.
set ( INC_FILES
    DllConfig.h
	MDFAddWorkspaceDialog.h
	MDFDataController.h
	MDFDatasetPlotData.h
	MDFEditLocalParameterDialog.h
	MDFFunctionPlotData.h
	MDFLocalParameterEditor.h
	MDFLocalParameterItemDelegate.h
	MDFLogValueFinder.h
	MDFPlotController.h
	MultiDatasetFit.h
)

set ( MOC_FILES 
    MultiDatasetFit.h
    MDFAddWorkspaceDialog.h
    MDFDataController.h
    MDFPlotController.h
    MDFLocalParameterEditor.h
    MDFLocalParameterItemDelegate.h
    MDFEditLocalParameterDialog.h
)

set ( UI_FILES 
   MultiDatasetFit.ui
   MDFAddWorkspaceDialog.ui
   EditLocalParameterDialog.ui
)

include_directories ( ../../qt/widgets/common/inc )

qt4_wrap_cpp ( MOCCED_FILES ${MOC_FILES} OPTIONS ${extra_options} )
qt4_add_resources ( RES_FILES_MULTIDATASETFIT ../../resources/icons/MultiDatasetFitIcons.qrc )

set ( ALL_SRC ${SRC_FILES} ${MOCCED_FILES} ${RES_FILES_MULTIDATASETFIT})

qt4_wrap_ui ( UI_HDRS ${UI_FILES} )
include_directories ( ${CMAKE_CURRENT_BINARY_DIR} )

# Use a precompiled header where they are supported
enable_precompiled_headers( PrecompiledHeader.h  ALL_SRC )
add_library ( MantidScientificInterfacesMultiDatasetFit ${ALL_SRC} ${INC_FILES} ${UI_HDRS} )

# Set the name of the generated library
set_target_properties ( MantidScientificInterfacesMultiDatasetFit PROPERTIES COMPILE_DEFINITIONS IN_MANTIDQT_MULTIDATASETFIT )
if ( MSVC_IDE )
#  # Add to the 'ScientificInterfaces' group in VS
  set_property ( TARGET MantidScientificInterfacesMultiDatasetFit PROPERTY FOLDER "ScientificInterfaces" )
endif()

if (OSX_VERSION VERSION_GREATER 10.8)
  set_target_properties ( MantidScientificInterfacesMultiDatasetFit PROPERTIES INSTALL_RPATH "@loader_path/../../../Contents/MacOS")
endif ()

find_Package(Threads)

target_link_libraries ( MantidScientificInterfacesMultiDatasetFit LINK_PRIVATE ${TCMALLOC_LIBRARIES_LINKTIME}
            MantidQtWidgetsCommonQt4
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

install ( TARGETS MantidScientificInterfacesMultiDatasetFit ${SYSTEM_PACKAGE_TARGET} DESTINATION ${PLUGINS_DIR}/qtplugins/mantid )
