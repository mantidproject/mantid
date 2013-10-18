set ( SRC_FILES
    src/LoadFlexiNexus.cpp
    src/PoldiRemoveDeadWires.cpp
    src/PoldiLoadChopperSlits.cpp
    src/PoldiLoadSpectra.cpp
    src/PoldiLoadIPP.cpp
    src/PoldiAutoCorrelation5.cpp
    src/PoldiLoadLog.cpp
    src/PoldiPeakDetection2.cpp
#   src/PoldiPeakFit.cpp
    src/InvertMDDim.cpp
    src/MDHistoToWorkspace2D.cpp
    src/ProjectMD.cpp
    src/SINQHMListener.cpp
    src/SliceMDHisto.cpp
    src/SINQTranspose3D.cpp
)

set ( INC_FILES
    inc/MantidSINQ/LoadFlexiNexus.h
    inc/MantidSINQ/PoldiRemoveDeadWires.h
    inc/MantidSINQ/PoldiLoadChopperSlits.h
    inc/MantidSINQ/PoldiLoadSpectra.h
    inc/MantidSINQ/PoldiLoadIPP.h
    inc/MantidSINQ/PoldiAutoCorrelation5.h
    inc/MantidSINQ/PoldiLoadLog.h
    inc/MantidSINQ/PoldiPeakDetection2.h
#   inc/MantidSINQ/PoldiPeakFit.h
    inc/MantidSINQ/InvertMDDim.h
    inc/MantidSINQ/MDHistoToWorkspace2D.h
    inc/MantidSINQ/ProjectMD.h
    inc/MantidSINQ/SINQHMListener.h
    inc/MantidSINQ/SliceMDHisto.h
    inc/MantidSINQ/SINQTranspose3D.h
)

set ( TEST_FILES
    LoadFlexiNexusTest.h
    ProjectMDTest.h
    InvertMDDimTest.h
    MDHistoToWorkspace2DTest.h
    SliceMDHistoTest.h
)


# Add a precompiled header where they are supported
enable_precompiled_headers ( inc/MantidSINQ/PrecompiledHeader.h SRC_FILES )
# Add the target for this directory
add_library ( SINQ ${SRC_FILES} ${INC_FILES} )
# Set the name of the generated library
set_target_properties ( SINQ PROPERTIES OUTPUT_NAME MantidSINQ )
# Add to the 'Framework' group in VS
set_property ( TARGET SINQ PROPERTY FOLDER "MantidFramework" )

include_directories ( inc ../MDEvents/inc ../CurveFitting/inc ) # todo: Need to remove CurveFitting link

target_link_libraries ( SINQ ${MANTIDLIBS} MDEvents CurveFitting )

# Add the unit tests directory
add_subdirectory ( test )

###########################################################################
# Installation settings
###########################################################################

install ( TARGETS SINQ ${SYSTEM_PACKAGE_TARGET} DESTINATION ${PLUGINS_DIR} )
