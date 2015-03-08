# Source files
set ( SRC_FILES
	#
	src/CalculateReflectometryQBase.cpp
	src/ConvToMDEventsWS.cpp
	src/ConvToMDHistoWS.cpp
	src/ConvToMDSelector.cpp
	src/ImportMDHistoWorkspaceBase.cpp
	src/IntegrateEllipsoids.cpp
	src/MDTransfNoQ.cpp
	src/MDWSTransform.cpp
	src/ReflectometryTransform.cpp
	src/UserFunctionMD.cpp
)

set ( SRC_UNITY_IGNORE_FILES 
    src/MDBoxBase.cpp
	src/MDBin.cpp
    src/MDBox.cpp
    src/MDBoxIterator.cpp
    src/MDEventWorkspace.cpp
    src/MDGridBox.cpp
)

# Header files
set ( INC_FILES
	#
	inc/MantidMDEvents/CalculateReflectometryQBase.h
	inc/MantidMDEvents/ConvToMDEventsWS.h
	inc/MantidMDEvents/ConvToMDHistoWS.h
	inc/MantidMDEvents/ConvToMDSelector.h
	inc/MantidMDEvents/ImportMDHistoWorkspaceBase.h
	inc/MantidMDEvents/IntegrateEllipsoids.h
	inc/MantidMDEvents/MDTransfInterface.h
	inc/MantidMDEvents/MDTransfNoQ.h
	inc/MantidMDEvents/MDWSTransform.h
	inc/MantidMDEvents/ReflectometryTransform.h
	inc/MantidMDEvents/UserFunctionMD.h
)

# Test files.
set ( TEST_FILES
	MDWSTransfTest.h
	SkippingPolicyTest.h
)


if(UNITY_BUILD)
  include(UnityBuild)
  enable_unity_build(MDEvents SRC_FILES SRC_UNITY_IGNORE_FILES 10)
endif(UNITY_BUILD)

# Use a precompiled header where they are supported
enable_precompiled_headers( inc/MantidMDEvents/PrecompiledHeader.h SRC_FILES )
# Add the target for this directory
add_library ( MDEvents ${SRC_FILES} ${INC_FILES})
# Set the name of the generated library
set_target_properties ( MDEvents PROPERTIES OUTPUT_NAME MantidMDEvents 
                                                 COMPILE_DEFINITIONS IN_MANTID_MDEVENT )

if (OSX_VERSION VERSION_GREATER 10.8)
  set_target_properties ( MDEvents PROPERTIES INSTALL_RPATH "@loader_path/../MacOS")
endif () 

# Intensive use of templated libaries can cause large objects to be generated. These require
# an additional flag in MSVC.
if ( MSVC )
  set_target_properties ( MDEvents PROPERTIES COMPILE_FLAGS "/bigobj" )
endif ()
                                                 
# Add to the 'Framework' group in VS
set_property ( TARGET MDEvents PROPERTY FOLDER "MantidFramework" )

include_directories( inc )
target_link_libraries ( MDEvents ${MANTIDLIBS} )

# Add the unit tests directory
add_subdirectory ( test )

###########################################################################
# Installation settings
###########################################################################

install ( TARGETS MDEvents ${SYSTEM_PACKAGE_TARGET} DESTINATION ${LIB_DIR} )
