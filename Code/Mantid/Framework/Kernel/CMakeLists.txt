set ( SRC_FILES
	src/ANN_complete.cpp
	src/ArrayBoundedValidator.cpp
	src/ArrayLengthValidator.cpp
	src/ArrayProperty.cpp
	src/Atom.cpp
	src/BinFinder.cpp
	src/CPUTimer.cpp
	src/CatalogInfo.cpp
	src/CompositeValidator.cpp
	src/ConfigService.cpp
	src/ChecksumHelper.cpp
	src/DataItem.cpp
	src/DateAndTime.cpp
	src/DateTimeValidator.cpp 
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
	src/FileDescriptor.cpp
	src/FileValidator.cpp
	src/FilterChannel.cpp
	src/FilteredTimeSeriesProperty.cpp
	src/FloatingPointComparison.cpp
	src/FreeBlock.cpp
	src/Glob.cpp
	src/IPropertyManager.cpp
	src/ISaveable.cpp
	src/InstrumentInfo.cpp
	src/InternetHelper.cpp
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
	src/Material.cpp
	src/Math/Distributions/BoseEinsteinDistribution.cpp
	src/Math/Distributions/HermitePolynomials.cpp
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
	src/ParaViewVersion.cpp
	src/ProgressBase.cpp
	src/ProgressText.cpp
	src/Property.cpp
	src/PropertyHistory.cpp
	src/PropertyManager.cpp
	src/PropertyManagerOwner.cpp
	src/PropertyWithValue.cpp
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
	src/UnitLabel.cpp
	src/UnitLabelTypes.cpp
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
	inc/MantidKernel/CatalogInfo.h
	inc/MantidKernel/CompositeValidator.h
	inc/MantidKernel/ConfigService.h
	inc/MantidKernel/ChecksumHelper.h
	inc/MantidKernel/DataItem.h
	inc/MantidKernel/DataService.h
	inc/MantidKernel/DateAndTime.h
	inc/MantidKernel/DateTimeValidator.h
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
	inc/MantidKernel/FileDescriptor.h
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
	inc/MantidKernel/ITimeSeriesProperty.h
	inc/MantidKernel/IValidator.h
	inc/MantidKernel/Instantiator.h
	inc/MantidKernel/InstrumentInfo.h
	inc/MantidKernel/InternetHelper.h
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
	inc/MantidKernel/Material.h
	inc/MantidKernel/Math/Distributions/BoseEinsteinDistribution.h
	inc/MantidKernel/Math/Distributions/HermitePolynomials.h
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
	inc/MantidKernel/NetworkProxy.h
	inc/MantidKernel/NeutronAtom.h
	inc/MantidKernel/NexusDescriptor.h
	inc/MantidKernel/NullValidator.h
	inc/MantidKernel/ParaViewVersion.h
	inc/MantidKernel/PhysicalConstants.h
	inc/MantidKernel/ProgressBase.h
	inc/MantidKernel/ProgressText.h
	inc/MantidKernel/Property.h
	inc/MantidKernel/PropertyHistory.h
	inc/MantidKernel/PropertyManager.h
	inc/MantidKernel/PropertyManagerOwner.h
	inc/MantidKernel/PropertyWithValue.h
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
	inc/MantidKernel/StartsWithValidator.h
	inc/MantidKernel/Statistics.h
	inc/MantidKernel/StdoutChannel.h
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
)

set ( TEST_FILES
	ArrayBoundedValidatorTest.h
	ArrayLengthValidatorTest.h
	ArrayPropertyTest.h
	AtomTest.h
	BinFinderTest.h
	BinaryFileTest.h
	BoseEinsteinDistributionTest.h
	BoundedValidatorTest.h
	CPUTimerTest.h
	CacheTest.h
	CatalogInfoTest.h
	CompositeValidatorTest.h
	ConfigServiceTest.h
	ChecksumHelperTest.h
	DataServiceTest.h
	DateAndTimeTest.h
	DateTimeValidatorTest.h
	DateValidatorTest.h
	DeltaEModeTest.h
	DirectoryValidatorTest.h
	DiskBufferISaveableTest.h
	DiskBufferTest.h
	DynamicFactoryTest.h
	EnabledWhenPropertyTest.h
	EnvironmentHistoryTest.h
	FacilitiesTest.h
	FileDescriptorTest.h
	FileValidatorTest.h
	FilterChannelTest.h
	FilteredTimeSeriesPropertyTest.h
	FloatingPointComparisonTest.h
	FreeBlockTest.h
	FunctionTaskTest.h
	GlobTest.h
	HermitePolynomialsTest.h
	IPropertySettingsTest.h
	ISaveableTest.h
	IValidatorTest.h
	InstrumentInfoTest.h
	InternetHelperTest.h
	InterpolationTest.h
	ListValidatorTest.h
	LogFilterTest.h
	LogParserTest.h
	LoggerTest.h
	MRUListTest.h
	MagneticFormFactorTableTest.h
	MagneticIonTest.h
	MandatoryValidatorTest.h
	MaskedPropertyTest.h
	MaterialTest.h
	MatrixPropertyTest.h
	MatrixTest.h
	MemoryTest.h
	MersenneTwisterTest.h
	MultiFileNameParserTest.h
	MultiFileValidatorTest.h
	MutexTest.h
	NDPseudoRandomNumberGeneratorTest.h
	NDRandomNumberGeneratorTest.h
	NeutronAtomTest.h
	NexusDescriptorTest.h
	NullValidatorTest.h
	ProgressBaseTest.h
	ProgressTextTest.h
	PropertyHistoryTest.h
	PropertyManagerTest.h
	PropertyTest.h
	PropertyWithValueTest.h
	ProxyInfoTest.h
	QuatTest.h
	ReadLockTest.h
	RebinHistogramTest.h
	RebinParamsValidatorTest.h
	RegexStringsTest.h
	SLSQPMinimizerTest.h
	SobolSequenceTest.h
	StartsWithValidatorTest.h
	StatisticsTest.h
	StdoutChannelTest.h
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
	UserStringParserTest.h
	UtilsTest.h
	V2DTest.h
	V3DTest.h
	VMDTest.h
	VectorHelperTest.h
	VisibleWhenPropertyTest.h
	WriteLockTest.h
)

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

include_directories ( ${GSL_INCLUDE_DIR} )

# Enable precompiled headers where supported (must be done before target is added)
enable_precompiled_headers ( inc/MantidKernel/PrecompiledHeader.h SRC_FILES )
# Add the target for this directory
add_library ( Kernel ${SRC_FILES} ${INC_FILES} )
# Set the name of the generated library
set_target_properties ( Kernel PROPERTIES OUTPUT_NAME MantidKernel
                                          COMPILE_DEFINITIONS "IN_MANTID_KERNEL;PSAPI_VERSION=1" )

# Add to the 'Framework' group in VS
set_property ( TARGET Kernel PROPERTY FOLDER "MantidFramework" )

# For older gcc versions (<4.5) if we do not put stdc++ first then things crash when an exceptions are
# thrown in Python.
if ( GCC_COMPILER_VERSION AND GCC_COMPILER_VERSION VERSION_LESS "4.5" )
 target_link_libraries ( Kernel stdc++ )
endif()
target_link_libraries ( Kernel ${NEXUS_LIBRARIES} ${MANTIDLIBS} ${GSL_LIBRARIES} 
                        ${NETWORK_LIBRARIES} ${JSONCPP_LIBRARIES} )
if ( WIN32 )
  target_link_libraries ( Kernel Psapi.lib ) # For memory usage queries
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
# This section deals with creating the ParaViewVersion implementation
###########################################################################
configure_file ( ${CMAKE_CURRENT_SOURCE_DIR}/src/ParaViewVersion.cpp.in
                 ${CMAKE_CURRENT_SOURCE_DIR}/src/ParaViewVersion.cpp
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

set ( PLUGINS "." )
set ( PV_PLUGINS "./pvplugins" )
set ( IGNORE_PARAVIEW "0" )
set ( QTPLUGINS "." )
set ( UPDATE_INSTRUMENT_DEFINTITIONS "0" )
set ( ENABLE_USAGE_REPORTS "0" )
set ( PYTHONPLUGIN_DIRS "${MANTID_ROOT}/Framework/PythonInterface/plugins" )
set ( DATADIRS ${ExternalData_BINARY_ROOT}/Testing/Data/UnitTest;${ExternalData_BINARY_ROOT}/Testing/Data/DocTest;${MANTID_ROOT}/instrument )
set ( COLORMAPS_FOLDER ${MANTID_ROOT}/Installers/colormaps/ )
set ( MANTIDPUBLISHER "http://upload.mantidproject.org/scriptrepository/payload/publish_debug" )
set ( HTML_ROOT ${DOCS_BUILDDIR}/html )

# Construct script paths.
set ( MANTID_SCRIPTS ${MANTID_ROOT}/scripts )
# First the required scripts variable...
# Omitting the following (as of now) empty directories as CMake doesn't copy them on install and you then end up with a warning message....
# @MANTID_ROOT@/scripts/Crystallography;@MANTID_ROOT@/scripts/DisorderedMaterials;@MANTID_ROOT@/scripts/Engineering;@MANTID_ROOT@/scripts/Muons;@MANTID_ROOT@/scripts/Neutrinos
set ( REQUIREDSCRIPT_SUBDIRS Engineering Inelastic Reflectometry SANS)

set ( REQUIREDSCRIPT_DIRS ${MANTID_ROOT}/scripts )
foreach ( SUBDIR ${REQUIREDSCRIPT_SUBDIRS} )
  set ( REQUIREDSCRIPT_DIRS "${REQUIREDSCRIPT_DIRS};${MANTID_SCRIPTS}/${SUBDIR}" )
endforeach()

# Second the standard scripts variable. The variable in the file is NOT recursive so add the top level directories from here
set ( EXCLUDED_DIRS test )
set ( PYTHONSCRIPT_DIRS "" )
file ( GLOB SCRIPT_SUBDIRS RELATIVE ${MANTID_SCRIPTS} ${MANTID_SCRIPTS}/* )
foreach ( SUBDIR ${SCRIPT_SUBDIRS} )
  set ( DIR ${MANTID_SCRIPTS}/${SUBDIR} )
  if ( IS_DIRECTORY ${DIR} AND NOT EXISTS ${DIR}/__init__.py )
    list ( FIND REQUIREDSCRIPT_SUBDIRS ${SUBDIR} HAVE_DIR )
    list ( FIND EXCLUDED_DIRS ${SUBDIR} EXCLUDE_DIR )
    if ( HAVE_DIR EQUAL -1 AND EXCLUDE_DIR EQUAL -1 ) # If it is not in REQUIRED and not EXCLUDED
      if ( PYTHONSCRIPT_DIRS )
        set ( PYTHONSCRIPT_DIRS "${PYTHONSCRIPT_DIRS};${DIR}" )
      else()
        set ( PYTHONSCRIPT_DIRS "${DIR}" ) # Avoid first ;
      endif()
    endif()
  endif()
endforeach()

# For an mpi-enabled build, don't log to file and format console output (which winds up in a file) differently
# These settings carry down to the installation configuration below
if ( MPI_BUILD )
  set ( FILECHANNEL NullChannel )
  set ( CONSOLEPATTERN "%H:%M:%S,%i [%N:%P] %p %s - %t")
  set ( PV_PLUGINS "" )
  set ( IGNORE_PARAVIEW "1" )
else ()
  set ( FILECHANNEL FileChannel )
  set ( CONSOLEPATTERN "%s-[%p] %t" )
endif ()

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
set ( MANTID_ROOT_BUILD ${MANTID_ROOT} )
if ( APPLE )
  set ( MANTID_ROOT ../.. )
else ()
  set ( MANTID_ROOT .. )
endif ()

set ( PLUGINS ${MANTID_ROOT}/plugins )
set ( PYTHONPLUGIN_DIRS "${PLUGINS}/python" )
set ( UPDATE_INSTRUMENT_DEFINTITIONS "1" )
set ( ENABLE_USAGE_REPORTS "1" )
set ( DATADIRS "" )
set ( MANTIDPUBLISHER "http://upload.mantidproject.org/scriptrepository/payload/publish" )
set ( HTML_ROOT ../share/doc/html )

# Construct script paths by replacing the old MANTID_ROOT with the new one.
# Unfortunately string (REGEX REPLACE ... )removes the semi-colons so we have to do this in a loop.

# First REQUIREDSCRIPT_DIRS
set ( WITH_SEMICOLONS "" )
foreach ( DIR ${REQUIREDSCRIPT_DIRS} )
  string ( REGEX REPLACE "${MANTID_ROOT_BUILD}" "${MANTID_ROOT}" DIR ${DIR} )
  if ( WITH_SEMICOLONS )
    set ( WITH_SEMICOLONS "${WITH_SEMICOLONS};${DIR}" )
  else()
    set ( WITH_SEMICOLONS "${DIR}" ) # Avoid first ;
  endif()
endforeach()
set ( REQUIREDSCRIPT_DIRS "${WITH_SEMICOLONS}" )

# Second PYTHONSCRIPT_DIRS
set ( WITH_SEMICOLONS "" )
foreach ( DIR ${PYTHONSCRIPT_DIRS} )
  string ( REGEX REPLACE "${MANTID_ROOT_BUILD}" "${MANTID_ROOT}" DIR ${DIR} )
  if ( WITH_SEMICOLONS )
    set ( WITH_SEMICOLONS "${WITH_SEMICOLONS};${DIR}" )
  else()
    set ( WITH_SEMICOLONS "${DIR}" ) # Avoid first ;
  endif()
endforeach()
set ( PYTHONSCRIPT_DIRS "${WITH_SEMICOLONS}" )

# For a framework-only (e.g. MPI) build some of these are not relevant and should
# be left empty to avoid warnings on starting Mantid
if ( ${CMAKE_PROJECT_NAME} MATCHES "MantidFramework" )
  set ( QTPLUGINS "" )
  set ( COLORMAPS_FOLDER "" )
  set ( PV_PLUGINS "" )
  set ( IGNORE_PARAVIEW "1" )
else ()
  set ( QTPLUGINS ${MANTID_ROOT}/plugins/qtplugins/mantid )
  set ( COLORMAPS_FOLDER ${MANTID_ROOT}/colormaps )
  set ( PV_PLUGINS ${MANTID_ROOT}/${PVPLUGINS_SUBDIR} )
endif ()

configure_file ( ../Properties/Mantid.properties.template
                 ${CMAKE_CURRENT_BINARY_DIR}/Mantid.properties.install
)
install ( FILES ${CMAKE_CURRENT_BINARY_DIR}/Mantid.properties.install
          DESTINATION ${BIN_DIR} RENAME Mantid.properties
)
