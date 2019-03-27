set ( SRC_FILES
	src/ANN_complete.cpp
	src/ArrayBoundedValidator.cpp
	src/ArrayLengthValidator.cpp
	src/ArrayOrderedPairsValidator.cpp
	src/ArrayProperty.cpp
	src/Atom.cpp
	src/BinFinder.cpp
	src/BinaryStreamReader.cpp
	src/CPUTimer.cpp
	src/CatalogInfo.cpp
	src/ChecksumHelper.cpp
	src/CompositeValidator.cpp
	src/ComputeResourceInfo.cpp
	src/ConfigService.cpp
	src/ConfigObserver.cpp
	src/ConfigPropertyObserver.cpp
	src/DateAndTime.cpp
	src/DataItem.cpp
	src/DateAndTimeHelpers.cpp
	src/DateTimeValidator.cpp
	src/DateValidator.cpp
	src/DeltaEMode.cpp
	src/Diffraction.cpp
	src/DirectoryValidator.cpp
	src/DiskBuffer.cpp
	src/DllOpen.cpp
	src/EnabledWhenProperty.cpp
	src/EnvironmentHistory.cpp
	src/EqualBinsChecker.cpp
	src/ErrorReporter.cpp
	src/Exception.cpp
	src/FacilityInfo.cpp
	src/FileDescriptor.cpp
	src/FileValidator.cpp
	src/FilteredTimeSeriesProperty.cpp
	src/FloatingPointComparison.cpp
	src/FreeBlock.cpp
	src/GitHubApiHelper.cpp
	src/Glob.cpp
	src/ICatalogInfo.cpp
	src/IPropertyManager.cpp
	src/ISaveable.cpp
	src/IndexSet.cpp
	src/InstrumentInfo.cpp
	src/InternetHelper.cpp
	src/Interpolation.cpp
	src/InvisibleProperty.cpp
	src/LibraryManager.cpp
	src/LibraryWrapper.cpp
	src/LiveListenerInfo.cpp
	src/LogFilter.cpp
	src/LogParser.cpp
	src/Logger.cpp
	src/MDAxisValidator.cpp
	src/MDUnit.cpp
	src/MDUnitFactory.cpp
	src/MagneticFormFactorTable.cpp
	src/MagneticIon.cpp
	src/MandatoryValidator.cpp
	src/MantidVersion.cpp
	src/MaskedProperty.cpp
	src/Material.cpp
	src/MaterialBuilder.cpp
	src/MaterialXMLParser.cpp
	src/Math/ChebyshevPolyFit.cpp
	src/Math/Distributions/BoseEinsteinDistribution.cpp
	src/Math/Distributions/ChebyshevPolynomial.cpp
	src/Math/Distributions/ChebyshevSeries.cpp
	src/Math/Optimization/SLSQPMinimizer.cpp
	src/Matrix.cpp
	src/MatrixProperty.cpp
	src/Memory.cpp
	src/MersenneTwister.cpp
	src/MultiFileNameParser.cpp
	src/MultiFileValidator.cpp
	src/NDRandomNumberGenerator.cpp
	src/NeutronAtom.cpp
	src/NexusDescriptor.cpp
	src/NullValidator.cpp
	src/OptionalBool.cpp
	src/ParaViewVersion.cpp
	src/ProgressBase.cpp
	src/Property.cpp
	src/PropertyHistory.cpp
	src/PropertyManager.cpp
	src/PropertyManagerDataService.cpp
	src/PropertyManagerOwner.cpp
	src/PropertyManagerProperty.cpp
	src/PropertyNexus.cpp
	src/PropertyWithValue.cpp
	src/PropertyWithValueJSON.cpp
	src/ProxyInfo.cpp
	src/PseudoRandomNumberGenerator.cpp
	src/Quat.cpp
	src/ReadLock.cpp
	src/RebinParamsValidator.cpp
	src/RegexStrings.cpp
	src/RemoteJobManager.cpp
	src/SingletonHolder.cpp
	src/SobolSequence.cpp
	src/StartsWithValidator.cpp
	src/Statistics.cpp
	src/StdoutChannel.cpp
	src/StringContainsValidator.cpp
	src/StringTokenizer.cpp
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
	src/UnitLabel.cpp
	src/UnitLabelTypes.cpp
	src/UsageService.cpp
	src/UserCatalogInfo.cpp
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
	inc/MantidKernel/ArrayOrderedPairsValidator.h
	inc/MantidKernel/ArrayProperty.h
	inc/MantidKernel/Atom.h
	inc/MantidKernel/BinFinder.h
	inc/MantidKernel/BinaryFile.h
	inc/MantidKernel/BinaryStreamReader.h
	inc/MantidKernel/BoundedValidator.h
	inc/MantidKernel/CPUTimer.h
	inc/MantidKernel/Cache.h
	inc/MantidKernel/CaseInsensitiveMap.h
	inc/MantidKernel/CatalogInfo.h
	inc/MantidKernel/Chainable.h
	inc/MantidKernel/ChainableFactory.h
	inc/MantidKernel/ChecksumHelper.h
	inc/MantidKernel/CompositeValidator.h
	inc/MantidKernel/ComputeResourceInfo.h
	inc/MantidKernel/ConfigService.h
	inc/MantidKernel/ConfigObserver.h
	inc/MantidKernel/ConfigPropertyObserver.h
	inc/MantidKernel/DateAndTime.h
	inc/MantidKernel/DataItem.h
	inc/MantidKernel/DataService.h
	inc/MantidKernel/DateAndTimeHelpers.h
	inc/MantidKernel/DateTimeValidator.h
	inc/MantidKernel/DateValidator.h
	inc/MantidKernel/DeltaEMode.h
	inc/MantidKernel/Diffraction.h
	inc/MantidKernel/DirectoryValidator.h
	inc/MantidKernel/DiskBuffer.h
	inc/MantidKernel/DllConfig.h
	inc/MantidKernel/DllOpen.h
	inc/MantidKernel/DocumentationHeader.h
	inc/MantidKernel/DynamicFactory.h
	inc/MantidKernel/EigenConversionHelpers.h
	inc/MantidKernel/EmptyValues.h
	inc/MantidKernel/EnabledWhenProperty.h
	inc/MantidKernel/EnvironmentHistory.h
	inc/MantidKernel/ErrorReporter.h
	inc/MantidKernel/EqualBinsChecker.h
	inc/MantidKernel/Exception.h
	inc/MantidKernel/FacilityInfo.h
	inc/MantidKernel/Fast_Exponential.h
	inc/MantidKernel/FileDescriptor.h
	inc/MantidKernel/FileValidator.h
	inc/MantidKernel/FilteredTimeSeriesProperty.h
	inc/MantidKernel/FloatingPointComparison.h
	inc/MantidKernel/FreeBlock.h
	inc/MantidKernel/FunctionTask.h
	inc/MantidKernel/GitHubApiHelper.h
	inc/MantidKernel/Glob.h
	inc/MantidKernel/ICatalogInfo.h
	inc/MantidKernel/IPropertyManager.h
	inc/MantidKernel/IPropertySettings.h
	inc/MantidKernel/ISaveable.h
	inc/MantidKernel/ITimeSeriesProperty.h
	inc/MantidKernel/IValidator.h
	inc/MantidKernel/IndexSet.h
	inc/MantidKernel/Instantiator.h
	inc/MantidKernel/InstrumentInfo.h
	inc/MantidKernel/InternetHelper.h
	inc/MantidKernel/Interpolation.h
	inc/MantidKernel/InvisibleProperty.h
	inc/MantidKernel/LibraryManager.h
	inc/MantidKernel/LibraryWrapper.h
	inc/MantidKernel/ListValidator.h
	inc/MantidKernel/LiveListenerInfo.h
	inc/MantidKernel/LogFilter.h
	inc/MantidKernel/LogParser.h
	inc/MantidKernel/Logger.h
	inc/MantidKernel/MDAxisValidator.h
	inc/MantidKernel/MDUnit.h
	inc/MantidKernel/MRUList.h
	inc/MantidKernel/MagneticFormFactorTable.h
	inc/MantidKernel/MagneticIon.h
	inc/MantidKernel/MandatoryValidator.h
	inc/MantidKernel/MantidVersion.h
	inc/MantidKernel/MaskedProperty.h
	inc/MantidKernel/Material.h
	inc/MantidKernel/MaterialBuilder.h
	inc/MantidKernel/MaterialXMLParser.h
	inc/MantidKernel/Math/ChebyshevPolyFit.h
	inc/MantidKernel/Math/Distributions/BoseEinsteinDistribution.h
	inc/MantidKernel/Math/Distributions/ChebyshevPolynomial.h
	inc/MantidKernel/Math/Distributions/ChebyshevSeries.h
	inc/MantidKernel/Math/Optimization/SLSQPMinimizer.h
	inc/MantidKernel/Matrix.h
	inc/MantidKernel/MatrixProperty.h
	inc/MantidKernel/Memory.h
	inc/MantidKernel/MersenneTwister.h
	inc/MantidKernel/MultiFileNameParser.h
	inc/MantidKernel/MultiFileValidator.h
	inc/MantidKernel/MultiThreaded.h
	inc/MantidKernel/NDPseudoRandomNumberGenerator.h
	inc/MantidKernel/NDRandomNumberGenerator.h
	inc/MantidKernel/NearestNeighbours.h
	inc/MantidKernel/NetworkProxy.h
	inc/MantidKernel/NeutronAtom.h
	inc/MantidKernel/NexusDescriptor.h
	inc/MantidKernel/normal_distribution.h
	inc/MantidKernel/NullValidator.h
	inc/MantidKernel/OptionalBool.h
	inc/MantidKernel/ParaViewVersion.h
	inc/MantidKernel/PhysicalConstants.h
	inc/MantidKernel/PocoVersion.h
	inc/MantidKernel/ProgressBase.h
	inc/MantidKernel/Property.h
	inc/MantidKernel/PropertyHelper.h
	inc/MantidKernel/PropertyHistory.h
	inc/MantidKernel/PropertyManager.h
	inc/MantidKernel/PropertyManagerDataService.h
	inc/MantidKernel/PropertyManagerOwner.h
	inc/MantidKernel/PropertyManagerProperty.h
	inc/MantidKernel/PropertyManager_fwd.h
	inc/MantidKernel/PropertyNexus.h
	inc/MantidKernel/PropertyWithValue.h
	inc/MantidKernel/PropertyWithValueJSON.h
	inc/MantidKernel/ProxyInfo.h
	inc/MantidKernel/PseudoRandomNumberGenerator.h
	inc/MantidKernel/QuasiRandomNumberSequence.h
	inc/MantidKernel/Quat.h
	inc/MantidKernel/ReadLock.h
	inc/MantidKernel/RebinParamsValidator.h
	inc/MantidKernel/RegexStrings.h
	inc/MantidKernel/RegistrationHelper.h
	inc/MantidKernel/RemoteJobManager.h
	inc/MantidKernel/SingletonHolder.h
	inc/MantidKernel/SobolSequence.h
	inc/MantidKernel/SpecialCoordinateSystem.h
	inc/MantidKernel/StartsWithValidator.h
	inc/MantidKernel/Statistics.h
	inc/MantidKernel/StdoutChannel.h
	inc/MantidKernel/StringContainsValidator.h
	inc/MantidKernel/StringTokenizer.h
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
	inc/MantidKernel/UnitLabel.h
	inc/MantidKernel/UnitLabelTypes.h
	inc/MantidKernel/UsageService.h
	inc/MantidKernel/UserCatalogInfo.h
	inc/MantidKernel/UserStringParser.h
	inc/MantidKernel/Utils.h
	inc/MantidKernel/V2D.h
	inc/MantidKernel/V3D.h
	inc/MantidKernel/VMD.h
	inc/MantidKernel/VectorHelper.h
	inc/MantidKernel/VisibleWhenProperty.h
	inc/MantidKernel/WarningSuppressions.h
	inc/MantidKernel/WriteLock.h
	inc/MantidKernel/XMLInstantiator.h
	inc/MantidKernel/cow_ptr.h
	inc/MantidKernel/make_cow.h
	inc/MantidKernel/make_unique.h
)

set ( TEST_FILES
	ArrayBoundedValidatorTest.h
	ArrayLengthValidatorTest.h
	ArrayOrderedPairsValidatorTest.h
	ArrayPropertyTest.h
	AtomTest.h
	BinFinderTest.h
	BinaryFileTest.h
	BinaryStreamReaderTest.h
	BoseEinsteinDistributionTest.h
	BoundedValidatorTest.h
	CPUTimerTest.h
	CacheTest.h
	CatalogInfoTest.h
	ChebyshevPolyFitTest.h
	ChebyshevPolynomialTest.h
	ChebyshevSeriesTest.h
	ChecksumHelperTest.h
	CompositeValidatorTest.h
	ComputeResourceInfoTest.h
	ConfigServiceTest.h
	ConfigObserverTest.h
	ConfigPropertyObserverTest.h
	CowPtrTest.h
	DataServiceTest.h
	DateAndTimeHelpersTest.h
	DateTimeValidatorTest.h
	DateValidatorTest.h
	DeltaEModeTest.h
	DiffractionTest.h
	DirectoryValidatorTest.h
	DiskBufferISaveableTest.h
	DiskBufferTest.h
	DllOpenTest.h
	DynamicFactoryTest.h
	EigenConversionHelpersTest.h
	EnabledWhenPropertyTest.h
	EnvironmentHistoryTest.h
	ErrorReporterTest.h
	EqualBinsCheckerTest.h
	FacilitiesTest.h
	FileDescriptorTest.h
	FileValidatorTest.h
	FilteredTimeSeriesPropertyTest.h
	FloatingPointComparisonTest.h
	FreeBlockTest.h
	FunctionTaskTest.h
	GlobTest.h
	IPropertySettingsTest.h
	ISaveableTest.h
	IValidatorTest.h
	IndexSetTest.h
	InstrumentInfoTest.h
	InternetHelperTest.h
	InterpolationTest.h
	InvisiblePropertyTest.h
	ListValidatorTest.h
	LiveListenerInfoTest.h
	LogFilterTest.h
	LogParserTest.h
	LoggerTest.h
	MDAxisValidatorTest.h
	MDUnitFactoryTest.h
	MDUnitTest.h
	MRUListTest.h
	MagneticFormFactorTableTest.h
	MagneticIonTest.h
	MakeCowTest.h
	MakeUniqueTest.h
	MandatoryValidatorTest.h
	MaskedPropertyTest.h
	MaterialBuilderTest.h
	MaterialTest.h
	MaterialXMLParserTest.h
	MatrixPropertyTest.h
	MatrixTest.h
	MemoryTest.h
	MersenneTwisterTest.h
	MultiFileNameParserTest.h
	MultiFileValidatorTest.h
	MutexTest.h
	NDPseudoRandomNumberGeneratorTest.h
	NDRandomNumberGeneratorTest.h
	NearestNeighboursTest.h
	NeutronAtomTest.h
	NexusDescriptorTest.h
	NullValidatorTest.h
	OptionalBoolTest.h
	ProgressBaseTest.h
	PropertyHistoryTest.h
	PropertyManagerDataServiceTest.h
	PropertyManagerPropertyTest.h
	PropertyManagerTest.h
	PropertyNexusTest.h
	PropertyTest.h
	PropertyWithValueTest.h
  PropertyWithValueJSONTest.h
	ProxyInfoTest.h
	QuatTest.h
	ReadLockTest.h
	RebinHistogramTest.h
	RebinParamsValidatorTest.h
	RegexStringsTest.h
	SLSQPMinimizerTest.h
	ShrinkToFitTest.h
	SobolSequenceTest.h
	SpecialCoordinateSystemTest.h
	StartsWithValidatorTest.h
	StatisticsTest.h
	StdoutChannelTest.h
	StringContainsValidatorTest.h
	StringTokenizerTest.h
	StringsTest.h
	TaskTest.h
	ThreadPoolRunnableTest.h
	ThreadPoolTest.h
	ThreadSchedulerMutexesTest.h
	ThreadSchedulerTest.h
	TimeSeriesPropertyTest.h
	TimeSplitterTest.h
	TimerTest.h
	TypedValidatorTest.h
	UnitConversionTest.h
	UnitFactoryTest.h
	UnitLabelTest.h
	UnitTest.h
	UsageServiceTest.h
	UserCatalogInfoTest.h
	UserStringParserTest.h
	UtilsTest.h
	V2DTest.h
	V3DTest.h
	VMDTest.h
	VectorHelperTest.h
	VisibleWhenPropertyTest.h
	WriteLockTest.h
)

if (COVERALLS)
    foreach( loop_var ${SRC_FILES} ${INC_FILES})
      set_property(GLOBAL APPEND PROPERTY COVERAGE_SRCS "${CMAKE_CURRENT_SOURCE_DIR}/${loop_var}")
    endforeach(loop_var)
endif()

if ( ${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
    LIST( APPEND SRC_FILES src/NetworkProxyOSX.cpp )
    SET( NETWORK_LIBRARIES "-framework SystemConfiguration" "-framework CoreFoundation"  "-framework CoreServices")
elseif (${CMAKE_SYSTEM_NAME} MATCHES "Windows")
    LIST( APPEND SRC_FILES src/NetworkProxyWin.cpp )
    SET( NETWORK_LIBRARIES Winhttp )
else()
    LIST( APPEND SRC_FILES src/NetworkProxyLinux.cpp )
endif()

if(UNITY_BUILD)
  include(UnityBuild)
  enable_unity_build(Kernel SRC_FILES SRC_UNITY_IGNORE_FILES 10)
endif(UNITY_BUILD)

# Enable precompiled headers where supported (must be done before target is added)
enable_precompiled_headers ( inc/MantidKernel/PrecompiledHeader.h SRC_FILES )
# Add the target for this directory
add_library ( Kernel ${SRC_FILES} ${INC_FILES} )
# Set the name of the generated library
set_target_properties ( Kernel PROPERTIES OUTPUT_NAME MantidKernel
                                          COMPILE_DEFINITIONS "IN_MANTID_KERNEL;PSAPI_VERSION=1" )

target_include_directories ( Kernel SYSTEM PUBLIC ${Boost_INCLUDE_DIRS} ${POCO_INCLUDE_DIRS} ${JSONCPP_INCLUDE_DIR}
  PRIVATE ${NEXUS_INCLUDE_DIR} ${GSL_INCLUDE_DIR} ${OPENSSL_INCLUDE_DIR} )

if (OSX_VERSION VERSION_GREATER 10.8)
  set_target_properties(Kernel PROPERTIES INSTALL_RPATH "@loader_path/../MacOS")
elseif ( ${CMAKE_SYSTEM_NAME} STREQUAL "Linux" )
  set_target_properties(Kernel PROPERTIES INSTALL_RPATH "\$ORIGIN/../${LIB_DIR}")
endif ()

# Add to the 'Framework' group in VS
set_property ( TARGET Kernel PROPERTY FOLDER "MantidFramework" )

# For older gcc versions (<4.5) if we do not put stdc++ first then things crash when an exceptions are
# thrown in Python.
if ( GCC_COMPILER_VERSION AND GCC_COMPILER_VERSION VERSION_LESS "4.5" )
 target_link_libraries ( Kernel LINK_PRIVATE stdc++ )
endif()
target_link_libraries ( Kernel
  LINK_PUBLIC ${NEXUS_LIBRARIES} Eigen3::Eigen ${JSONCPP_LIBRARIES} ${TBB_LIBRARIES} ${TBB_MALLOC_LIBRARIES}
  LINK_PRIVATE ${TCMALLOC_LIBRARIES_LINKTIME} ${GSL_LIBRARIES} ${MANTIDLIBS} ${NETWORK_LIBRARIES} )
if ( WIN32 )
  target_link_libraries ( Kernel LINK_PRIVATE Psapi.lib ) # For memory usage queries
endif()

# Add the unit tests directory
add_subdirectory ( test )

###########################################################################
# This section deals with creating the MantidVersion implementation
###########################################################################

# DON'T try to update the MantidVersion source file if we were not able to
# determine it from git and the file already exists. This makes sure the
# version number is correct when building off the source tarball

if ( NOT NOT_GIT_REPO OR NOT EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/src/MantidVersion.cpp )
  configure_file ( ${CMAKE_CURRENT_SOURCE_DIR}/src/MantidVersion.cpp.in
                   ${CMAKE_CURRENT_SOURCE_DIR}/src/MantidVersion.cpp
  )
endif ()

###########################################################################
# This section deals with creating the PocoVersion implementation
###########################################################################
configure_file ( ${CMAKE_CURRENT_SOURCE_DIR}/inc/MantidKernel/PocoVersion.h.in
                 ${CMAKE_CURRENT_SOURCE_DIR}/inc/MantidKernel/PocoVersion.h
)

###########################################################################
# This section deals with creating the ParaViewVersion implementation
###########################################################################
configure_file ( ${CMAKE_CURRENT_SOURCE_DIR}/src/ParaViewVersion.cpp.in
                 ${CMAKE_CURRENT_SOURCE_DIR}/src/ParaViewVersion.cpp
)
###########################################################################
# This section deals with creating the GithubApiHelper implementation
###########################################################################

if ( WIN32 )
  set ( GITHUB_AUTHORIZATION_TOKEN "8ec7afc857540ee60af78cba1cf7779a6ed0b6b9")
elseif ( APPLE )
  set ( GITHUB_AUTHORIZATION_TOKEN "9f1c1acd61ecb87b21ad0382f6b403c8294992c5")
else()
  set ( GITHUB_AUTHORIZATION_TOKEN "ccacbaf39a7ad8151fca03b4ea99a29b28f0993b")
endif()
configure_file ( ${CMAKE_CURRENT_SOURCE_DIR}/inc/MantidKernel/GitHubApiHelper.h.in
                 ${CMAKE_CURRENT_SOURCE_DIR}/inc/MantidKernel/GitHubApiHelper.h
)

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

set ( FRAMEWORK_PLUGINS_DIR "." )
set ( QT_PLUGINS_DIR "." )
# %V will be replaced with the major version of Qt at runtime
if ( MAKE_VATES )
  set ( PV_PLUGINS_DIR "./plugins/paraview/qt%V" )
  if ( MSVC )
    set (PARAVIEW_PYTHON_PATHS "${ParaView_DIR}/bin/$<$<CONFIG:Release>:Release>$<$<CONFIG:Debug>:Debug>;${ParaView_DIR}/lib/$<$<CONFIG:Release>:Release>$<$<CONFIG:Debug>:Debug>;${ParaView_DIR}/lib/site-packages;")
  else()
    set ( PARAVIEW_PYTHON_PATHS "${ParaView_DIR}/lib;${ParaView_DIR}/lib/site-packages;")
  endif()
else ()
  set ( PV_PLUGINS_DIR "" )
  set ( PARAVIEW_PYTHON_PATHS "")
endif ()
set ( IGNORE_PARAVIEW "0" )
set ( UPDATE_INSTRUMENT_DEFINTITIONS "0" )
set ( CHECK_FOR_NEW_MANTID_VERSION "0" )
set ( ENABLE_USAGE_REPORTS "0" )
set ( PYTHONPLUGIN_DIRS "${MANTID_ROOT}/Framework/PythonInterface/plugins" )
set ( DATADIRS ${ExternalData_BINARY_ROOT}/Testing/Data/UnitTest;${ExternalData_BINARY_ROOT}/Testing/Data/DocTest;${MANTID_ROOT}/instrument )
set ( COLORMAPS_FOLDER ${MANTID_ROOT}/installers/colormaps/ )
set ( MANTIDPUBLISHER "http://upload.mantidproject.org/scriptrepository?debug=1" )
set ( HTML_ROOT ${DOCS_BUILDDIR}/html )

# For an mpi-enabled build, do not log to file, format console output (which winds up in a file) differently
# These settings carry down to the installation configuration below
if ( MPI_BUILD )
  set ( CONSOLEPATTERN "%H:%M:%S,%i [%N:%P] %p %s - %t")
  set ( PV_PLUGINS_DIR "" )
  set ( IGNORE_PARAVIEW "1" )
else ()
  set ( CONSOLEPATTERN "%s-[%p] %t" )
endif ()

set ( CONSOLECHANNELCLASS "ConsoleChannel" )
configure_file ( ../Properties/Mantid.properties.template
                 ${CMAKE_CURRENT_BINARY_DIR}/Mantid.properties.config
)

 # Then, copy into appropriate place
if(MSVC)
  file(GENERATE
     OUTPUT
     ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/$<CONFIG>/Mantid.properties
     INPUT
     ${CMAKE_CURRENT_BINARY_DIR}/Mantid.properties.config
  )
else()
  file(GENERATE
     OUTPUT
     ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/Mantid.properties
     INPUT
     ${CMAKE_CURRENT_BINARY_DIR}/Mantid.properties.config
  )
endif()

###########################################################################
# Installation settings
###########################################################################

mtd_install_targets( TARGETS Kernel INSTALL_DIRS ${LIB_DIR} ${WORKBENCH_LIB_DIR})

# Create the properties file for the installer
set ( MANTID_ROOT_BUILD ${MANTID_ROOT} )
if ( APPLE AND ENABLE_MANTIDPLOT )
  set ( MANTID_ROOT ../.. )
else ()
  set ( MANTID_ROOT .. )
endif ()

# Log to file for installed packages but not mpi
if ( MPI_BUILD OR MPI_EXPERIMENTAL )
  set ( ENABLE_NETWORK_ACCESS 0 )
else ()
  set ( ENABLE_NETWORK_ACCESS 1 )
endif ()
set ( FRAMEWORK_PLUGINS_DIR ${MANTID_ROOT}/plugins )
set ( PYTHONPLUGIN_DIRS "${FRAMEWORK_PLUGINS_DIR}/python" )
if(MAKE_VATES)
  if (APPLE)
    set ( PARAVIEW_PYTHON_PATHS "../Libraries;../Python;")
  elseif (WIN32)
    set ( PV_LIBS "../lib/paraview-${PARAVIEW_VERSION_MAJOR}.${PARAVIEW_VERSION_MINOR}" )
    set ( PARAVIEW_PYTHON_PATHS "${PV_LIBS}/site-packages;" )
  else () #Linux
    set ( PV_LIBS "${CMAKE_INSTALL_PREFIX}/lib/paraview-${PARAVIEW_VERSION}")
    set ( PARAVIEW_PYTHON_PATHS "${PV_LIBS};${PV_LIBS}/site-packages;")
  endif ()
else ()
    set ( PARAVIEW_PYTHON_PATHS "")
endif()
set ( UPDATE_INSTRUMENT_DEFINTITIONS "${ENABLE_NETWORK_ACCESS}" )
set ( CHECK_FOR_NEW_MANTID_VERSION "${ENABLE_NETWORK_ACCESS}" )
set ( ENABLE_USAGE_REPORTS "${ENABLE_NETWORK_ACCESS}" )
set ( DATADIRS "" )
set ( MANTIDPUBLISHER "http://upload.mantidproject.org/scriptrepository" )
set ( HTML_ROOT ../share/doc/html )

# For a framework-only (e.g. MPI) build some of these are not relevant and should
# be left empty to avoid warnings on starting Mantid
if ( ${CMAKE_PROJECT_NAME} MATCHES "MantidFramework" )
  set ( QT_PLUGINS_DIR "" )
  set ( COLORMAPS_FOLDER "" )
  set ( PV_PLUGINS_DIR "" )
  set ( IGNORE_PARAVIEW "1" )
else ()
  set ( QT_PLUGINS_DIR "${MANTID_ROOT}/plugins/qt%V" )
  set ( PV_PLUGINS_DIR "${MANTID_ROOT}/plugins/${PVPLUGINS_SUBDIR}/qt%V" )
  set ( COLORMAPS_FOLDER ${MANTID_ROOT}/colormaps )
endif ()

set ( CONSOLECHANNELCLASS "StdoutChannel" )
configure_file ( ../Properties/Mantid.properties.template
                 ${CMAKE_CURRENT_BINARY_DIR}/Mantid.properties.install
)
install ( FILES ${CMAKE_CURRENT_BINARY_DIR}/Mantid.properties.install
          DESTINATION ${BIN_DIR} RENAME Mantid.properties )

install ( FILES ${CMAKE_CURRENT_BINARY_DIR}/Mantid.properties.install
          DESTINATION ${WORKBENCH_BIN_DIR} RENAME Mantid.properties )
