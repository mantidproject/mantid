set(SRC_FILES
    src/ANN_complete.cpp
    src/ArrayBoundedValidator.cpp
    src/ArrayLengthValidator.cpp
    src/ArrayOrderedPairsValidator.cpp
    src/ArrayProperty.cpp
    src/Atom.cpp
    src/AttenuationProfile.cpp
    src/BinFinder.cpp
    src/BinaryStreamReader.cpp
    src/BinaryStreamWriter.cpp
    src/CPUTimer.cpp
    src/CatalogInfo.cpp
    src/ChecksumHelper.cpp
    src/CompositeValidator.cpp
    src/ConfigObserver.cpp
    src/ConfigPropertyObserver.cpp
    src/ConfigService.cpp
    src/DataItem.cpp
    src/DateAndTime.cpp
    src/DateAndTimeHelpers.cpp
    src/DateTimeValidator.cpp
    src/DateValidator.cpp
    src/DeltaEMode.cpp
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
    src/MagneticIon.cpp
    src/MandatoryValidator.cpp
    src/MantidVersion.cpp
    src/MaskedProperty.cpp
    src/Material.cpp
    src/MaterialBuilder.cpp
    src/MaterialXMLParser.cpp
    src/Math/ChebyshevPolyFit.cpp
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
    src/NexusHDF5Descriptor.cpp
    src/NullValidator.cpp
    src/OptionalBool.cpp
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
    src/SetValueWhenProperty.cpp
    src/SingletonHolder.cpp
    src/SobolSequence.cpp
    src/SplittingInterval.cpp
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
    src/TimeROI.cpp
    src/TimeSeriesProperty.cpp
    src/Timer.cpp
    src/TopicInfo.cpp
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

set(INC_FILES
    inc/MantidKernel/ANN/ANN.h
    inc/MantidKernel/ANN/ANNperf.h
    inc/MantidKernel/ANN/ANNx.h
    inc/MantidKernel/ArrayBoundedValidator.h
    inc/MantidKernel/ArrayLengthValidator.h
    inc/MantidKernel/ArrayOrderedPairsValidator.h
    inc/MantidKernel/ArrayProperty.h
    inc/MantidKernel/Atom.h
    inc/MantidKernel/AttenuationProfile.h
    inc/MantidKernel/BinFinder.h
    inc/MantidKernel/BinaryFile.h
    inc/MantidKernel/BinaryStreamReader.h
    inc/MantidKernel/BinaryStreamWriter.h
    inc/MantidKernel/BoundedValidator.h
    inc/MantidKernel/CPUTimer.h
    inc/MantidKernel/Cache.h
    inc/MantidKernel/CaseInsensitiveMap.h
    inc/MantidKernel/CatalogInfo.h
    inc/MantidKernel/Chainable.h
    inc/MantidKernel/ChainableFactory.h
    inc/MantidKernel/ChecksumHelper.h
    inc/MantidKernel/CompositeValidator.h
    inc/MantidKernel/ConfigObserver.h
    inc/MantidKernel/ConfigPropertyObserver.h
    inc/MantidKernel/ConfigService.h
    inc/MantidKernel/DataItem.h
    inc/MantidKernel/DataService.h
    inc/MantidKernel/DateAndTime.h
    inc/MantidKernel/DateAndTimeHelpers.h
    inc/MantidKernel/DateTimeValidator.h
    inc/MantidKernel/DateValidator.h
    inc/MantidKernel/DeltaEMode.h
    inc/MantidKernel/DirectoryValidator.h
    inc/MantidKernel/DiskBuffer.h
    inc/MantidKernel/DllOpen.h
    inc/MantidKernel/DocumentationHeader.h
    inc/MantidKernel/DynamicFactory.h
    inc/MantidKernel/DynamicPointerCastHelper.h
    inc/MantidKernel/EigenConversionHelpers.h
    inc/MantidKernel/EmptyValues.h
    inc/MantidKernel/EnabledWhenProperty.h
    inc/MantidKernel/EnumeratedString.h
    inc/MantidKernel/EnumeratedStringProperty.h
    inc/MantidKernel/EnvironmentHistory.h
    inc/MantidKernel/EqualBinsChecker.h
    inc/MantidKernel/ErrorReporter.h
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
    inc/MantidKernel/LambdaValidator.h
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
    inc/MantidKernel/MagneticIon.h
    inc/MantidKernel/MandatoryValidator.h
    inc/MantidKernel/MantidVersion.h
    inc/MantidKernel/MaskedProperty.h
    inc/MantidKernel/Material.h
    inc/MantidKernel/MaterialBuilder.h
    inc/MantidKernel/MaterialXMLParser.h
    inc/MantidKernel/Math/ChebyshevPolyFit.h
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
    inc/MantidKernel/NexusHDF5Descriptor.h
    inc/MantidKernel/NullValidator.h
    inc/MantidKernel/OptionalBool.h
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
    inc/MantidKernel/SetValueWhenProperty.h
    inc/MantidKernel/SingletonHolder.h
    inc/MantidKernel/SobolSequence.h
    inc/MantidKernel/SpecialCoordinateSystem.h
    inc/MantidKernel/SplittingInterval.h
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
    inc/MantidKernel/TimeROI.h
    inc/MantidKernel/TimeSeriesProperty.h
    inc/MantidKernel/Timer.h
    inc/MantidKernel/Tolerance.h
    inc/MantidKernel/TopicInfo.h
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
    inc/MantidKernel/cow_ptr.h
    inc/MantidKernel/make_cow.h
    inc/MantidKernel/normal_distribution.h
)

set(TEST_FILES
    ArrayBoundedValidatorTest.h
    ArrayLengthValidatorTest.h
    ArrayOrderedPairsValidatorTest.h
    ArrayPropertyTest.h
    AtomTest.h
    AttenuationProfileTest.h
    BinFinderTest.h
    BinaryFileTest.h
    BinaryStreamReaderTest.h
    BinaryStreamWriterTest.h
    BoundedValidatorTest.h
    CPUTimerTest.h
    CacheTest.h
    CatalogInfoTest.h
    ChebyshevPolyFitTest.h
    ChebyshevPolynomialTest.h
    ChebyshevSeriesTest.h
    ChecksumHelperTest.h
    CompositeValidatorTest.h
    ConfigObserverTest.h
    ConfigPropertyObserverTest.h
    ConfigServiceTest.h
    CowPtrTest.h
    DataServiceTest.h
    DateAndTimeHelpersTest.h
    DateTimeValidatorTest.h
    DateValidatorTest.h
    DeltaEModeTest.h
    DirectoryValidatorTest.h
    DiskBufferISaveableTest.h
    DiskBufferTest.h
    DllOpenTest.h
    DynamicFactoryTest.h
    DynamicPointerCastHelperTest.h
    EigenConversionHelpersTest.h
    EnabledWhenPropertyTest.h
    EnumeratedStringPropertyTest.h
    EnumeratedStringTest.h
    EnvironmentHistoryTest.h
    EqualBinsCheckerTest.h
    ErrorReporterTest.h
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
    LambdaValidatorTest.h
    ListValidatorTest.h
    LiveListenerInfoTest.h
    LogFilterTest.h
    LogParserTest.h
    LoggerTest.h
    MDAxisValidatorTest.h
    MDUnitFactoryTest.h
    MDUnitTest.h
    MRUListTest.h
    MagneticIonTest.h
    MakeCowTest.h
    MandatoryValidatorTest.h
    MantidVersionTest.h
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
    NexusHDF5DescriptorTest.h
    NullValidatorTest.h
    OptionalBoolTest.h
    ProgressBaseTest.h
    PropertyHistoryTest.h
    PropertyManagerDataServiceTest.h
    PropertyManagerPropertyTest.h
    PropertyManagerTest.h
    PropertyNexusTest.h
    PropertyTest.h
    PropertyWithValueJSONTest.h
    PropertyWithValueTest.h
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
    SplittingIntervalTest.h
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
    TimeIntervalTest.h
    TimeROITest.h
    TimeSeriesPropertyTest.h
    TimerTest.h
    TopicInfoTest.h
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

if(COVERAGE)
  foreach(loop_var ${SRC_FILES} ${INC_FILES})
    set_property(GLOBAL APPEND PROPERTY COVERAGE_SRCS "${CMAKE_CURRENT_SOURCE_DIR}/${loop_var}")
  endforeach(loop_var)
endif()

if(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
  list(APPEND SRC_FILES src/NetworkProxyOSX.cpp)
  set(NETWORK_LIBRARIES "-framework SystemConfiguration" "-framework CoreFoundation" "-framework CoreServices")
elseif(${CMAKE_SYSTEM_NAME} MATCHES "Windows")
  list(APPEND SRC_FILES src/NetworkProxyWin.cpp)
  set(NETWORK_LIBRARIES Winhttp)
else()
  list(APPEND SRC_FILES src/NetworkProxyLinux.cpp)
endif()

# Enable precompiled headers where supported (must be done before target is added)
enable_precompiled_headers(inc/MantidKernel/PrecompiledHeader.h SRC_FILES)
# Add the target for this directory
add_library(Kernel ${SRC_FILES} ${INC_FILES})
add_library(Mantid::Kernel ALIAS Kernel)
# Set the name of the generated library
set_target_properties(Kernel PROPERTIES OUTPUT_NAME MantidKernel COMPILE_DEFINITIONS "IN_MANTID_KERNEL;PSAPI_VERSION=1")

target_include_directories(
  Kernel PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/inc> $<INSTALL_INTERFACE:include/Mantid>
)

if(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
  set_target_properties(Kernel PROPERTIES INSTALL_RPATH "@loader_path/../MacOS;@loader_path/../Frameworks")
elseif(${CMAKE_SYSTEM_NAME} STREQUAL "Linux")
  set_target_properties(Kernel PROPERTIES INSTALL_RPATH "\$ORIGIN/../${LIB_DIR}")
endif()

# Add to the 'Framework' group in VS
set_property(TARGET Kernel PROPERTY FOLDER "MantidFramework")
target_compile_features(Kernel PUBLIC cxx_std_17)

target_link_libraries(
  Kernel
  PUBLIC Mantid::Types
         Mantid::Json
         Nexus::nexuscpp
         Eigen3::Eigen
         TBB::tbb
         Poco::Foundation
         Poco::Net
         Poco::XML
         Poco::Util
         Poco::NetSSL
         GSL::gsl
         OpenSSL::SSL
         ${HDF5_LIBRARIES}
         ${HDF5_HL_LIBRARIES}
  PRIVATE ${NETWORK_LIBRARIES}
)
if(WIN32)
  target_link_libraries(Kernel PRIVATE Psapi.lib) # For memory usage queries
endif()

# H5_BUILT_AS_DYNAMIC_LIB required https://github.com/conda-forge/hdf5-feedstock/issues/58
target_compile_definitions(Kernel PUBLIC -DH5_BUILT_AS_DYNAMIC_LIB)

# Add the unit tests directory
add_subdirectory(test)

# Auto-generate exports header
target_include_directories(
  Kernel PUBLIC $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/Framework/Kernel> $<INSTALL_INTERFACE:include/Mantid>
)
generate_mantid_export_header(Kernel TRUE)

# This section deals with creating the MantidVersion implementation

# DON'T try to update the MantidVersion source file if we were not able to determine it from git and the file already
# exists. This makes sure the version number is correct when building off the source tarball

if(NOT NOT_GIT_REPO OR NOT EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/src/MantidVersion.cpp)
  configure_file(${CMAKE_CURRENT_SOURCE_DIR}/src/MantidVersion.cpp.in ${CMAKE_CURRENT_SOURCE_DIR}/src/MantidVersion.cpp)
  configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/test/MantidVersionTest.h.in ${CMAKE_CURRENT_SOURCE_DIR}/test/MantidVersionTest.h
  )
endif()

# This section deals with creating the PocoVersion implementation
configure_file(
  ${CMAKE_CURRENT_SOURCE_DIR}/inc/MantidKernel/PocoVersion.h.in
  ${CMAKE_CURRENT_SOURCE_DIR}/inc/MantidKernel/PocoVersion.h
)

# This section deals with creating the GithubApiHelper implementation Defaults to 40-character string to allocate the
# appropriate amount of space in the final binary for it to be replaced if necessary
set(GITHUB_AUTHORIZATION_TOKEN
    "unsetunsetunsetunsetunsetunsetunsetunset"
    CACHE STRING "token used for authenticated read access to github"
)
configure_file(
  ${CMAKE_CURRENT_SOURCE_DIR}/inc/MantidKernel/GitHubApiHelper.h.in
  ${CMAKE_CURRENT_SOURCE_DIR}/inc/MantidKernel/GitHubApiHelper.h
)

# Manipulate the Mantid.properties file to work wherever you build to

# This needs to be done as a 2-step process for it to work for VS builds First the part that turns relative into
# absolute paths in the properties file
set(MANTID_ROOT ${CMAKE_SOURCE_DIR})
# This keeps things working for people building just the framework project
set(PYTHON_INTERFACES_DIR ${MANTID_ROOT}/qt/python/mantidqtinterfaces/mantidqtinterfaces)
if(${CMAKE_PROJECT_NAME} STREQUAL "MantidFramework")
  set(MANTID_ROOT ${CMAKE_SOURCE_DIR}/..)
endif()

set(FRAMEWORK_PLUGINS_DIR ".")
set(QT_PLUGINS_DIR ".")
# %V will be replaced with the major version of Qt at runtime
set(PV_PLUGINS_DIR "")
set(UPDATE_INSTRUMENT_DEFINTITIONS "0")
set(CHECK_FOR_NEW_MANTID_VERSION "0")
set(ENABLE_USAGE_REPORTS "0")
set(PYTHONPLUGIN_DIRS "${MANTID_ROOT}/Framework/PythonInterface/plugins")
set(PYTHONPLUGIN_MANIFEST "${CMAKE_BINARY_DIR}/Framework/PythonInterface/plugins/python-plugin-manifest.txt")
set(DATADIRS
    ${ExternalData_BINARY_ROOT}/Testing/Data/UnitTest;${ExternalData_BINARY_ROOT}/Testing/Data/DocTest;${MANTID_ROOT}/instrument
)
set(COLORMAPS_FOLDER ${MANTID_ROOT}/installers/colormaps/)
set(MANTIDPUBLISHER "https://upload.mantidproject.org/scriptrepository?debug=1")
set(HTML_ROOT ${DOCS_BUILDDIR}/html)

set(CONSOLEPATTERN "%s-[%p] %t")
set(CONSOLECHANNELCLASS "ConsoleChannel")
set(ALIASDEPRECATED "Raise")
set(ALGORITHMDEPRECATED "Raise")
configure_file(../Properties/Mantid.properties.template ${CMAKE_CURRENT_BINARY_DIR}/Mantid.properties.config)

# Then, copy into appropriate place
get_property(_is_multi_config GLOBAL PROPERTY GENERATOR_IS_MULTI_CONFIG)
if(_is_multi_config)
  file(
    GENERATE
    OUTPUT ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/$<CONFIG>/Mantid.properties
    INPUT ${CMAKE_CURRENT_BINARY_DIR}/Mantid.properties.config
  )
else()
  file(
    GENERATE
    OUTPUT ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/Mantid.properties
    INPUT ${CMAKE_CURRENT_BINARY_DIR}/Mantid.properties.config
  )
endif()

# Create the properties file for the installer
set(MANTID_ROOT_BUILD ${MANTID_ROOT})
set(MANTID_ROOT ..)

set(ENABLE_NETWORK_ACCESS 1)
set(FRAMEWORK_PLUGINS_DIR ${MANTID_ROOT}/plugins)
set(PYTHONPLUGIN_DIRS "${FRAMEWORK_PLUGINS_DIR}/python")
set(PYTHONPLUGIN_MANIFEST "${PYTHONPLUGIN_DIRS}/python-plugin-manifest.txt")
set(UPDATE_INSTRUMENT_DEFINTITIONS "${ENABLE_NETWORK_ACCESS}")
set(CHECK_FOR_NEW_MANTID_VERSION "${ENABLE_NETWORK_ACCESS}")
set(ENABLE_USAGE_REPORTS "${ENABLE_NETWORK_ACCESS}")
set(DATADIRS "")
set(MANTIDPUBLISHER "https://upload.mantidproject.org/scriptrepository")
set(HTML_ROOT ../share/doc/html)

# For a framework-only (e.g. MPI) build some of these are not relevant and should be left empty to avoid warnings on
# starting Mantid
if(${CMAKE_PROJECT_NAME} MATCHES "MantidFramework")
  set(QT_PLUGINS_DIR "")
  set(COLORMAPS_FOLDER "")
else()
  set(QT_PLUGINS_DIR "${MANTID_ROOT}/plugins/qt%V")
  set(COLORMAPS_FOLDER ${MANTID_ROOT}/colormaps)
endif()

set(CONSOLECHANNELCLASS "StdoutChannel")
set(ALIASDEPRECATED "Log")
set(ALGORITHMDEPRECATED "Log")
configure_file(../Properties/Mantid.properties.template ${CMAKE_CURRENT_BINARY_DIR}/Mantid.properties.install)

mtd_install_files(
  FILES
  ${CMAKE_CURRENT_BINARY_DIR}/Mantid.properties.install
  INSTALL_DIRS
  ${BIN_DIR}
  ${WORKBENCH_BIN_DIR}
  RENAME
  Mantid.properties
)

# Installation settings
set(TARGET_EXPORT_NAME "MantidKernelTargets")
mtd_install_framework_lib(TARGETS Kernel INSTALL_EXPORT_FILE EXPORT_NAME ${TARGET_EXPORT_NAME})
