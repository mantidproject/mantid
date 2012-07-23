set ( SRC_FILES
	src/ANN_complete.cpp
	src/ArrayBoundedValidator.cpp
	src/ArrayLengthValidator.cpp
	src/ArrayProperty.cpp
	src/Atom.cpp
	src/BinFinder.cpp
	src/CPUTimer.cpp
	src/CompositeValidator.cpp
	src/ConfigService.cpp
	src/DataItem.cpp
	src/DateAndTime.cpp
	src/DateValidator.cpp
	src/DeltaEMode.cpp
	src/DirectoryValidator.cpp
	src/DiskBuffer.cpp
	src/DllOpen.cpp
	src/EmptyValues.cpp
	src/EnabledWhenProperty.cpp
	src/EnvironmentHistory.cpp
	src/Exception.cpp
	src/FacilityInfo.cpp
	src/FileValidator.cpp
	src/FilterChannel.cpp
	src/FilteredTimeSeriesProperty.cpp
	src/FloatingPointComparison.cpp
	src/FreeBlock.cpp
	src/Glob.cpp
	src/IPropertyManager.cpp
	src/ISaveable.cpp
	src/InstrumentInfo.cpp
	src/Interpolation.cpp
	src/LibraryManager.cpp
	src/LibraryWrapper.cpp
	src/ListValidator.cpp
	src/LogFilter.cpp
	src/LogParser.cpp
	src/Logger.cpp
	src/MagneticFormFactorTable.cpp
	src/MagneticIon.cpp
	src/MandatoryValidator.cpp
	src/MantidVersion.cpp
	src/MaskedProperty.cpp
	src/Math/Distributions/BoseEinsteinDistribution.cpp
	src/Matrix.cpp
	src/MatrixProperty.cpp
	src/Memory.cpp
	src/MersenneTwister.cpp
	src/MultiFileNameParser.cpp
	src/MultiFileValidator.cpp
	src/NDPseudoRandomNumberGenerator.cpp
	src/NDRandomNumberGenerator.cpp
	src/NeutronAtom.cpp
	src/NexusTestHelper.cpp
	src/ProgressBase.cpp
	src/ProgressText.cpp
	src/Property.cpp
	src/PropertyHistory.cpp
	src/PropertyManager.cpp
	src/PropertyManagerOwner.cpp
	src/PropertyWithValue.cpp
	src/PseudoRandomNumberGenerator.cpp
	src/Quat.cpp
	src/ReadLock.cpp
	src/RebinParamsValidator.cpp
	src/RegexStrings.cpp
	src/SignalChannel.cpp
	src/SingletonHolder.cpp
	src/SobolSequence.cpp
	src/Statistics.cpp
	src/Strings.cpp
	src/TestChannel.cpp
	src/ThreadPool.cpp
	src/ThreadPoolRunnable.cpp
	src/ThreadSafeLogStream.cpp
	src/TimeSeriesProperty.cpp
	src/TimeSplitter.cpp
	src/Timer.cpp
	src/Unit.cpp
	src/UnitConversion.cpp
	src/UnitFactory.cpp
	src/UserStringParser.cpp
	src/Utils.cpp
	src/V2D.cpp
	src/V3D.cpp
	src/VMD.cpp
	src/VectorHelper.cpp
	src/VisibleWhenProperty.cpp
	src/WriteLock.cpp
)

set ( SRC_UNITY_IGNORE_FILES src/Atom.cpp
        src/NeutronAtom.cpp
        src/FacilityInfo.cpp
        src/FilterChannel.cpp
        src/FileValidator.cpp
        src/DirectoryValidator.cpp
        src/PropertyManager.cpp
        src/Unit.cpp 
        src/System.cpp
        )

set ( INC_FILES
	inc/MantidKernel/ANN/ANN.h
	inc/MantidKernel/ANN/ANNperf.h
	inc/MantidKernel/ANN/ANNx.h
	inc/MantidKernel/ArrayBoundedValidator.h
	inc/MantidKernel/ArrayLengthValidator.h
	inc/MantidKernel/ArrayProperty.h
	inc/MantidKernel/Atom.h
	inc/MantidKernel/BinFinder.h
	inc/MantidKernel/BinaryFile.h
	inc/MantidKernel/BoundedValidator.h
	inc/MantidKernel/CPUTimer.h
	inc/MantidKernel/Cache.h
	inc/MantidKernel/CompositeValidator.h
	inc/MantidKernel/ConfigService.h
	inc/MantidKernel/DataItem.h
	inc/MantidKernel/DataService.h
	inc/MantidKernel/DateAndTime.h
	inc/MantidKernel/DateValidator.h
	inc/MantidKernel/DeltaEMode.h
	inc/MantidKernel/DirectoryValidator.h
	inc/MantidKernel/DiskBuffer.h
	inc/MantidKernel/DllConfig.h
	inc/MantidKernel/DllOpen.h
	inc/MantidKernel/DocumentationHeader.h
	inc/MantidKernel/DynamicFactory.h
	inc/MantidKernel/EmptyValues.h
	inc/MantidKernel/EnabledWhenProperty.h
	inc/MantidKernel/EnvironmentHistory.h
	inc/MantidKernel/Exception.h
	inc/MantidKernel/FacilityInfo.h
	inc/MantidKernel/Fast_Exponential.h
	inc/MantidKernel/FileValidator.h
	inc/MantidKernel/FilterChannel.h
	inc/MantidKernel/FilteredTimeSeriesProperty.h
	inc/MantidKernel/FloatingPointComparison.h
	inc/MantidKernel/FreeBlock.h
	inc/MantidKernel/FunctionTask.h
	inc/MantidKernel/Glob.h
	inc/MantidKernel/IPropertyManager.h
	inc/MantidKernel/IPropertySettings.h
	inc/MantidKernel/ISaveable.h
	inc/MantidKernel/IValidator.h
	inc/MantidKernel/Instantiator.h
	inc/MantidKernel/InstrumentInfo.h
	inc/MantidKernel/Interpolation.h
	inc/MantidKernel/LibraryManager.h
	inc/MantidKernel/LibraryWrapper.h
	inc/MantidKernel/ListValidator.h
	inc/MantidKernel/LogFilter.h
	inc/MantidKernel/LogParser.h
	inc/MantidKernel/Logger.h
	inc/MantidKernel/MRUList.h
	inc/MantidKernel/MagneticFormFactorTable.h
	inc/MantidKernel/MagneticIon.h
	inc/MantidKernel/MandatoryValidator.h
	inc/MantidKernel/MantidVersion.h
	inc/MantidKernel/MaskedProperty.h
	inc/MantidKernel/Math/Distributions/BoseEinsteinDistribution.h
	inc/MantidKernel/Matrix.h
	inc/MantidKernel/MatrixProperty.h
	inc/MantidKernel/Memory.h
	inc/MantidKernel/MersenneTwister.h
	inc/MantidKernel/MultiFileNameParser.h
	inc/MantidKernel/MultiFileValidator.h
	inc/MantidKernel/MultiThreaded.h
	inc/MantidKernel/NDPseudoRandomNumberGenerator.h
	inc/MantidKernel/NDRandomNumberGenerator.h
	inc/MantidKernel/NeutronAtom.h
	inc/MantidKernel/NexusTestHelper.h
	inc/MantidKernel/NullValidator.h
	inc/MantidKernel/PhysicalConstants.h
	inc/MantidKernel/ProgressBase.h
	inc/MantidKernel/ProgressText.h
	inc/MantidKernel/Property.h
	inc/MantidKernel/PropertyHistory.h
	inc/MantidKernel/PropertyManager.h
	inc/MantidKernel/PropertyManagerOwner.h
	inc/MantidKernel/PropertyWithValue.h
	inc/MantidKernel/PseudoRandomNumberGenerator.h
	inc/MantidKernel/QuasiRandomNumberSequence.h
	inc/MantidKernel/Quat.h
	inc/MantidKernel/ReadLock.h
	inc/MantidKernel/RebinParamsValidator.h
	inc/MantidKernel/RegexStrings.h
	inc/MantidKernel/RegistrationHelper.h
	inc/MantidKernel/SignalChannel.h
	inc/MantidKernel/SingletonHolder.h
	inc/MantidKernel/SobolSequence.h
	inc/MantidKernel/Statistics.h
	inc/MantidKernel/Strings.h
	inc/MantidKernel/System.h
	inc/MantidKernel/Task.h
	inc/MantidKernel/TestChannel.h
	inc/MantidKernel/ThreadPool.h
	inc/MantidKernel/ThreadPoolRunnable.h
	inc/MantidKernel/ThreadSafeLogStream.h
	inc/MantidKernel/ThreadScheduler.h
	inc/MantidKernel/ThreadSchedulerMutexes.h
	inc/MantidKernel/TimeSeriesProperty.h
	inc/MantidKernel/TimeSplitter.h
	inc/MantidKernel/Timer.h
	inc/MantidKernel/Tolerance.h
	inc/MantidKernel/TypedValidator.h
	inc/MantidKernel/Unit.h
	inc/MantidKernel/UnitConversion.h
	inc/MantidKernel/UnitFactory.h
	inc/MantidKernel/UserStringParser.h
	inc/MantidKernel/Utils.h
	inc/MantidKernel/V2D.h
	inc/MantidKernel/V3D.h
	inc/MantidKernel/VMD.h
	inc/MantidKernel/VectorHelper.h
	inc/MantidKernel/VisibleWhenProperty.h
	inc/MantidKernel/WriteLock.h
	inc/MantidKernel/cow_ptr.h
)

set ( TEST_FILES
	test/ArrayBoundedValidatorTest.h
	test/ArrayLengthValidatorTest.h
	test/ArrayPropertyTest.h
	test/AtomTest.h
	test/BinFinderTest.h
	test/BinaryFileTest.h
	test/BoseEinsteinDistributionTest.h
	test/BoundedValidatorTest.h
	test/CPUTimerTest.h
	test/CacheTest.h
	test/CompositeValidatorTest.h
	test/ConfigServiceTest.h
	test/DataServiceTest.h
	test/DateAndTimeTest.h
	test/DateValidatorTest.h
	test/DeltaEModeTest.h
	test/DirectoryValidatorTest.h
	test/DiskBufferTest.h
	test/DynamicFactoryTest.h
	test/EnabledWhenPropertyTest.h
	test/EnvironmentHistoryTest.h
	test/FacilitiesTest.h
	test/FileValidatorTest.h
	test/FilterChannelTest.h
	test/FilteredTimeSeriesPropertyTest.h
	test/FloatingPointComparisonTest.h
	test/FreeBlockTest.h
	test/FunctionTaskTest.h
	test/GlobTest.h
	test/IPropertySettingsTest.h
	test/ISaveableTest.h
	test/IValidatorTest.h
	test/InstrumentInfoTest.h
	test/InterpolationTest.h
	test/ListValidatorTest.h
	test/LogFilterTest.h
	test/LogParserTest.h
	test/LoggerTest.h
	test/MRUListTest.h
	test/MagneticFormFactorTableTest.h
	test/MagneticIonTest.h
	test/MandatoryValidatorTest.h
	test/MaskedPropertyTest.h
	test/MatrixPropertyTest.h
	test/MatrixTest.h
	test/MemoryTest.h
	test/MersenneTwisterTest.h
	test/MultiFileNameParserTest.h
	test/MultiFileValidatorTest.h
	test/MutexTest.h
	test/NDPseudoRandomNumberGeneratorTest.h
	test/NDRandomNumberGeneratorTest.h
	test/NeutronAtomTest.h
	test/NullValidatorTest.h
	test/ProgressBaseTest.h
	test/ProgressTextTest.h
	test/PropertyHistoryTest.h
	test/PropertyManagerTest.h
	test/PropertyTest.h
	test/PropertyWithValueTest.h
	test/QuatTest.h
	test/ReadLockTest.h
	test/RebinHistogramTest.h
	test/RebinParamsValidatorTest.h
	test/RegexStringsTest.h
	test/SignalChannelTest.h
	test/SobolSequenceTest.h
	test/StatisticsTest.h
	test/StringsTest.h
	test/TaskTest.h
	test/ThreadPoolRunnableTest.h
	test/ThreadPoolTest.h
	test/ThreadSchedulerMutexesTest.h
	test/ThreadSchedulerTest.h
	test/TimeSeriesPropertyTest.h
	test/TimeSplitterTest.h
	test/TimerTest.h
	test/TypedValidatorTest.h
	test/UnitConversionTest.h
	test/UnitFactoryTest.h
	test/UnitTest.h
	test/UserStringParserTest.h
	test/UtilsTest.h
	test/V2DTest.h
	test/V3DTest.h
	test/VMDTest.h
	test/VectorHelperTest.h
	test/VisibleWhenPropertyTest.h
	test/WriteLockTest.h
)

if(UNITY_BUILD)
  include(UnityBuild)
  enable_unity_build(Kernel SRC_FILES SRC_UNITY_IGNORE_FILES 10)
endif(UNITY_BUILD)

include_directories ( ${GSL_INCLUDE_DIR} )

# Add a precompiled header where they are supported
ADD_PRECOMPILED_HEADER ( inc/MantidKernel/PrecompiledHeader.h MantidKernel src/PrecompiledHeader.cpp SRC_FILES INC_FILES )
# Add the target for this directory
add_library ( Kernel ${SRC_FILES} ${INC_FILES} )
# Set the name of the generated library
set_target_properties ( Kernel PROPERTIES OUTPUT_NAME MantidKernel 
                                          COMPILE_DEFINITIONS "IN_MANTID_KERNEL;PSAPI_VERSION=1" )
# Add to the 'Framework' group in VS
set_property ( TARGET Kernel PROPERTY FOLDER "MantidFramework" )

target_link_libraries ( Kernel ${MANTIDLIBS} ${GSL_LIBRARIES} )
if ( WIN32 )
  target_link_libraries ( Kernel Psapi.lib ) # For memory usage queries
endif()

if ( CXXTEST_FOUND )
  cxxtest_add_test ( KernelTest ${TEST_FILES} )
  include_directories ( inc test )
  target_link_libraries( KernelTest Kernel ${GMOCK_LIBRARIES} ${GTEST_LIBRARIES} )
  add_dependencies ( FrameworkTests KernelTest )
  # Add to the 'FrameworkTests' group in VS
  set_property ( TARGET KernelTest PROPERTY FOLDER "UnitTests" )

  add_custom_command ( TARGET KernelTest POST_BUILD 
                       COMMAND ${CMAKE_COMMAND} ARGS -E copy_if_different 
                         ${CMAKE_CURRENT_SOURCE_DIR}/../Properties/MantidTest.properties
                         ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/${CMAKE_CFG_INTDIR}
                       COMMAND ${CMAKE_COMMAND} ARGS -E copy_if_different 
                         ${CMAKE_CURRENT_SOURCE_DIR}/../Properties/MantidTest.user.properties
                         ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/${CMAKE_CFG_INTDIR}
  )

endif ()

###########################################################################
# This section deals with creating the MantidVersion implementation
###########################################################################

# DON'T try to update the MantidVersion source file we were not able to
# determine it from git and the file already exists. This makes sure the 
# version number is correct when building off the source tarball

if ( NOT NOT_GIT_REPO OR NOT EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/src/MantidVersion.cpp )
  configure_file ( ${CMAKE_CURRENT_SOURCE_DIR}/src/MantidVersion.cpp.in
                   ${CMAKE_CURRENT_SOURCE_DIR}/src/MantidVersion.cpp
  )
  configure_file ( ${CMAKE_CURRENT_SOURCE_DIR}/../../Build/CMake/PatchVersionNumber.cmake.in
                   ${CMAKE_CURRENT_SOURCE_DIR}/../../Build/CMake/PatchVersionNumber.cmake
  )
endif ()

###########################################################################
# Manipulate the Mantid.properties file to work wherever you build to
###########################################################################

# This needs to be done as a 2-step process for it to work for VS builds
# First the part that turns relative into absolute paths in the properties file
set ( MANTID_ROOT ${CMAKE_SOURCE_DIR} )
# This keeps things working for people building just the framework project
if ( ${CMAKE_PROJECT_NAME} STREQUAL "MantidFramework" )
  set ( MANTID_ROOT ${CMAKE_SOURCE_DIR}/.. )
endif ()

set ( PLUGINS "." )
set ( PV_PLUGINS "./pvplugins" )
set ( QTPLUGINS "." )
set ( PYTHONALGS ${MANTID_ROOT}/Framework/PythonAPI/PythonAlgorithms )
set ( DATADIRS ${MANTID_ROOT}/../../Test/AutoTestData;${MANTID_ROOT}/instrument )
set ( COLORMAPS_FOLDER ${MANTID_ROOT}/Installers/colormaps/ )

configure_file ( ../Properties/Mantid.properties.template
                 ${CMAKE_CURRENT_BINARY_DIR}/Mantid.properties
)

# Then, at build time, copy into appropriate place
add_custom_command ( TARGET Kernel POST_BUILD
                     COMMAND ${CMAKE_COMMAND} ARGS -E copy_if_different 
                       ${CMAKE_CURRENT_BINARY_DIR}/Mantid.properties
                       ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/${CMAKE_CFG_INTDIR}
)

###########################################################################
# Installation settings
###########################################################################

install ( TARGETS Kernel ${SYSTEM_PACKAGE_TARGET} DESTINATION ${LIB_DIR} )

# Create the properties file for the installer
if ( APPLE )
  set ( MANTID_ROOT ../.. )
else ()
  set ( MANTID_ROOT .. )
endif ()

set ( PLUGINS ${MANTID_ROOT}/plugins )
set ( QTPLUGINS ${MANTID_ROOT}/plugins/qtplugins/mantid )
set ( COLORMAPS_FOLDER ${MANTID_ROOT}/colormaps )
set ( PV_PLUGINS ${MANTID_ROOT}/${PVPLUGINS_SUBDIR} )

set ( PYTHONALGS ${PLUGINS}/PythonAlgs )
set ( DATADIRS "" )
  
configure_file ( ../Properties/Mantid.properties.template
                 ${CMAKE_CURRENT_BINARY_DIR}/Mantid.properties.install
)
install ( FILES ${CMAKE_CURRENT_BINARY_DIR}/Mantid.properties.install 
          DESTINATION ${BIN_DIR} RENAME Mantid.properties
)
