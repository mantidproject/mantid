set ( SRC_FILES src/ArrayBoundedValidator.cpp
                src/ArrayProperty.cpp
                src/Atom.cpp
		src/BinFinder.cpp
		src/ConfigService.cpp
                src/DateAndTime.cpp
		src/DateValidator.cpp
                src/DirectoryValidator.cpp
		src/DllOpen.cpp
                src/EnvironmentHistory.cpp
		src/Exception.cpp
                src/FacilityInfo.cpp
		src/FileValidator.cpp
                src/FilterChannel.cpp
		src/Glob.cpp
                src/InstrumentInfo.cpp
		src/Interpolation.cpp
                src/IPropertyManager.cpp
		src/LibraryManager.cpp
                src/LibraryWrapper.cpp
		src/ListValidator.cpp
                src/LogFilter.cpp
		src/Logger.cpp
		src/LogParser.cpp
                src/MandatoryValidator.cpp
		src/MaskedProperty.cpp
                src/MersenneTwister.cpp
		src/NeutronAtom.cpp
                src/Property.cpp
		src/PropertyHistory.cpp
                src/PropertyManager.cpp
		src/PropertyManagerOwner.cpp
                src/PropertyWithValue.cpp
		src/RebinParamsValidator.cpp
                src/SignalChannel.cpp
		src/SingletonHolder.cpp
                src/Statistics.cpp
		src/SupportBasic.cpp
                src/SupportKernel.cpp
		src/System.cpp
                src/ThreadSafeLogStream.cpp
		src/Timer.cpp
                src/TimeSeriesProperty.cpp
		src/TimeSplitter.cpp
                src/Unit.cpp
		src/UnitFactory.cpp
		src/VectorHelper.cpp
		src/ANN/ANN.cpp
		src/ANN/bd_fix_rad_search.cpp
		src/ANN/bd_pr_search.cpp
		src/ANN/bd_search.cpp
		src/ANN/bd_tree.cpp
		src/ANN/bd_tree.h
		src/ANN/brute.cpp
		src/ANN/kd_dump.cpp
		src/ANN/kd_fix_rad_search.cpp
		src/ANN/kd_fix_rad_search.h
		src/ANN/kd_pr_search.cpp
		src/ANN/kd_pr_search.h
		src/ANN/kd_search.cpp
		src/ANN/kd_search.h
		src/ANN/kd_split.cpp
		src/ANN/kd_split.h
		src/ANN/kd_tree.cpp
		src/ANN/kd_tree.h
		src/ANN/kd_util.cpp
		src/ANN/kd_util.h
		src/ANN/perf.cpp
		src/ANN/pr_queue.h
		src/ANN/pr_queue_k.h )

set ( INC_FILES inc/MantidKernel/ArrayBoundedValidator.h
                inc/MantidKernel/ArrayProperty.h
		inc/MantidKernel/Atom.h
		inc/MantidKernel/BinaryFile.h
		inc/MantidKernel/BinFinder.h
		inc/MantidKernel/BoundedValidator.h
		inc/MantidKernel/Cache.h
		inc/MantidKernel/ConfigService.h
		inc/MantidKernel/cow_ptr.h
		inc/MantidKernel/DataService.h
		inc/MantidKernel/DateAndTime.h
		inc/MantidKernel/DateValidator.h
		inc/MantidKernel/DirectoryValidator.h
		inc/MantidKernel/DllExport.h
		inc/MantidKernel/DllOpen.h
		inc/MantidKernel/DocumentationHeader.h
		inc/MantidKernel/DynamicFactory.h
		inc/MantidKernel/EnvironmentHistory.h
		inc/MantidKernel/Exception.h
		inc/MantidKernel/FacilityInfo.h
		inc/MantidKernel/Fast_Exponential.h
		inc/MantidKernel/FileValidator.h
		inc/MantidKernel/FilterChannel.h
		inc/MantidKernel/Glob.h
		inc/MantidKernel/Instantiator.h
		inc/MantidKernel/InstrumentInfo.h
		inc/MantidKernel/Interpolation.h
		inc/MantidKernel/IPropertyManager.h
		inc/MantidKernel/IValidator.h
		inc/MantidKernel/LibraryManager.h
		inc/MantidKernel/LibraryWrapper.h
		inc/MantidKernel/ListValidator.h
		inc/MantidKernel/LogFilter.h
		inc/MantidKernel/Logger.h
		inc/MantidKernel/LogParser.h
		inc/MantidKernel/MandatoryValidator.h
		inc/MantidKernel/MantidVersion.h
		inc/MantidKernel/MaskedProperty.h
		inc/MantidKernel/MersenneTwister.h
		inc/MantidKernel/MRUList.h
		inc/MantidKernel/MultiThreaded.h
		inc/MantidKernel/NeutronAtom.h
		inc/MantidKernel/NullValidator.h
		inc/MantidKernel/PhysicalConstants.h
		inc/MantidKernel/Property.h
		inc/MantidKernel/PropertyHistory.h
		inc/MantidKernel/PropertyManager.h
		inc/MantidKernel/PropertyManagerOwner.h
		inc/MantidKernel/PropertyWithValue.h
		inc/MantidKernel/RandomNumberGenerator.h
		inc/MantidKernel/RebinParamsValidator.h
		inc/MantidKernel/SignalChannel.h
		inc/MantidKernel/SingletonHolder.h
		inc/MantidKernel/Statistics.h
		inc/MantidKernel/Support.h
		inc/MantidKernel/SupportTempCode.h
		inc/MantidKernel/System.h
		inc/MantidKernel/ThreadSafeLogStream.h
		inc/MantidKernel/Timer.h
		inc/MantidKernel/TimeSeriesProperty.h
		inc/MantidKernel/TimeSplitter.h
		inc/MantidKernel/UnitFactory.h
		inc/MantidKernel/Unit.h
		inc/MantidKernel/VectorHelper.h
		inc/MantidKernel/ANN/ANN.h
		inc/MantidKernel/ANN/ANNperf.h
		inc/MantidKernel/ANN/ANNx.h )

set ( TEST_FILES test/ArrayPropertyTest.h 
                 #test/BinaryFileTest.h
                 test/BinFinderTest.h 
                 test/BoundedValidatorTest.h )

# For Windows:
add_definitions ( -DIN_MANTID_KERNEL )

# Add the target for this directory
add_library ( Kernel ${SRC_FILES} ${INC_FILES})
# Set the name of the generated library
set_target_properties ( Kernel PROPERTIES OUTPUT_NAME MantidKernel )
# Add to the 'Framework' group in VS
set_property ( TARGET Kernel PROPERTY FOLDER "Framework" )

target_link_libraries ( Kernel ${MANTIDLIBS} )


add_custom_command ( TARGET Kernel POST_BUILD 
                     COMMAND ${CMAKE_COMMAND}
                     ARGS -E copy_if_different 
                       ${CMAKE_CURRENT_SOURCE_DIR}/../Properties/Mantid.properties
                       ${CMAKE_LIBRARY_OUTPUT_DIRECTORY} 
)

if ( CXXTEST_FOUND )
  cxxtest_add_test ( KernelTest ${TEST_FILES} )
  target_link_libraries( KernelTest Kernel )
endif ()
