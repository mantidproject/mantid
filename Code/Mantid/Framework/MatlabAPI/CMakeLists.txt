set ( SRC_FILES
	src/MatlabInterface.cpp 
    src/mwdebug_silent.c)

set ( INC_FILES )

if (COVERALLS)
    foreach( loop_var ${SRC_FILES} ${INC_FILES})
      set_property(GLOBAL APPEND PROPERTY COVERAGE_SRCS "${CMAKE_CURRENT_SOURCE_DIR}/${loop_var}")
    endforeach(loop_var)
endif()

add_definitions ( -DIN_MANTID_MATLAB_API -DMATLAB_MEX_FILE=1 )

include_directories ( inc )
include_directories ( ${MATLAB_INCLUDE_DIR} ${MATLAB_EXTERN_SRC} )
add_library ( MatlabAPI ${SRC_FILES} ${INC_FILES})
# Set the name of the generated library
set_target_properties ( MatlabAPI PROPERTIES OUTPUT_NAME MantidMatlabAPI )
# Add to the 'Framework' group in VS
set_property ( TARGET MatlabAPI PROPERTY FOLDER "MantidFramework" )
# Target dependencies
target_link_libraries ( MatlabAPI ${MANTIDLIBS} ${MATLAB_LIBRARIES} )

