set ( SRC_FILES
	DataComparison.cpp
	DirectConvertToEnergy.cpp
	Homer.cpp
	MantidEV.cpp
	MantidEVWorker.cpp
	SampleTransmission.cpp
	StepScan.cpp
	UserInputValidator.cpp
	background.cpp
	deltaECalc.cpp
)

# Include files aren't required, but this makes them appear in Visual Studio
# IMPORTANT: Include files are required in the MOC_FILES set. Scroll down to find it.
set ( INC_FILES
	Background.h
	DataComparison.h
	DirectConvertToEnergy.h
	DllConfig.h
	Homer.h
	MantidEV.h
	MantidEVWorker.h
	SampleTransmission.h
	StepScan.h
	Updateable.h
	UserInputValidator.h
	deltaECalc.h
)

set ( MOC_FILES 
    Background.h
    DataComparison.h
    DirectConvertToEnergy.h
    Homer.h
    MantidEV.h
    SampleTransmission.h
    StepScan.h
)

set ( UI_FILES 
    DataComparison.ui
    DirectConvertToEnergy.ui
    MantidEV.ui
    SampleTransmission.ui
    StepScan.ui
)


qt4_wrap_cpp ( MOCCED_FILES ${MOC_FILES} OPTIONS ${extra_options} )

set ( ALL_SRC ${SRC_FILES} ${MOCCED_FILES} )

qt4_wrap_ui ( UI_HDRS ${UI_FILES} )
include_directories ( ${CMAKE_CURRENT_BINARY_DIR} )

# Use a precompiled header where they are supported
enable_precompiled_headers( PrecompiledHeader.h  ALL_SRC )
add_library ( MantidScientificInterfacesGeneral ${ALL_SRC} ${INC_FILES} ${UI_HDRS} )

# Set the name of the generated library
set_target_properties ( MantidScientificInterfacesGeneral PROPERTIES COMPILE_DEFINITIONS IN_MANTIDQT_INTERFACESGENERAL )
if ( MSVC_IDE )
#  # Add to the 'ScientificInterfaces' group in VS
  set_property ( TARGET MantidScientificInterfacesGeneral PROPERTY FOLDER "ScientificInterfaces" )
endif()

if (OSX_VERSION VERSION_GREATER 10.8)
  set_target_properties ( MantidScientificInterfacesGeneral PROPERTIES INSTALL_RPATH "@loader_path/../../../Contents/MacOS")
endif ()

find_Package(Threads)

target_link_libraries ( MantidScientificInterfacesGeneral LINK_PRIVATE ${TCMALLOC_LIBRARIES_LINKTIME}
            MantidQtWidgetsCommon
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

install ( TARGETS MantidScientificInterfacesGeneral ${SYSTEM_PACKAGE_TARGET} DESTINATION ${PLUGINS_DIR}/qtplugins/mantid )
