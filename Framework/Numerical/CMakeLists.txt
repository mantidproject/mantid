set(SRC_FILES
    src/FloatingPointComparison.cpp
    src/Math/ChebyshevPolyFit.cpp
    src/Math/Distributions/ChebyshevPolynomial.cpp
    src/Math/Distributions/ChebyshevSeries.cpp
    src/Math/Optimization/SLSQPMinimizer.cpp
    src/Matrix.cpp
    src/MersenneTwister.cpp
    src/ParallelMinMax.cpp
    src/PseudoRandomNumberGenerator.cpp
)

set(INC_FILES
    inc/MantidNumerical/FloatingPointComparison.h
    inc/MantidNumerical/Math/ChebyshevPolyFit.h
    inc/MantidNumerical/Math/Distributions/ChebyshevPolynomial.h
    inc/MantidNumerical/Math/Distributions/ChebyshevSeries.h
    inc/MantidNumerical/Math/Optimization/SLSQPMinimizer.h
    inc/MantidNumerical/Matrix.h
)

set(TEST_FILES
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

# Add the target for this directory
add_library(Numerical ${SRC_FILES} ${INC_FILES})
add_library(Mantid::Numerical ALIAS Numerical)
# Set the name of the generated library
set_target_properties(Numerical PROPERTIES OUTPUT_NAME MantidNumerical COMPILE_DEFINITIONS "IN_MANTID_NUMERICAL;PSAPI_VERSION=1")

# Enable precompiled headers
target_precompile_headers(Numerical PRIVATE inc/MantidNumerical/PrecompiledHeader.h)

target_include_directories(
  Numerical PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/inc> $<INSTALL_INTERFACE:include/Mantid>
)

if(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
  set_target_properties(Numerical PROPERTIES INSTALL_RPATH "@loader_path/../MacOS;@loader_path/../Frameworks")
elseif(${CMAKE_SYSTEM_NAME} STREQUAL "Linux")
  set_target_properties(Numerical PROPERTIES INSTALL_RPATH "\$ORIGIN/../${LIB_DIR}")
endif()

# Add to the 'Framework' group in VS
set_property(TARGET Numerical PROPERTY FOLDER "MantidFramework")
target_compile_features(Numerical PUBLIC cxx_std_17)

target_link_libraries(
  Numerical
  PUBLIC Mantid::Types
         Mantid::Json
         Mantid::Nexus
         Mantid::LegacyNexus
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
  PRIVATE ${NETWORK_LIBRARIES}
)
if(WIN32)
  target_link_libraries(Numerical PRIVATE Psapi.lib) # For memory usage queries
endif()

# H5_BUILT_AS_DYNAMIC_LIB required https://github.com/conda-forge/hdf5-feedstock/issues/58
target_compile_definitions(Numerical PUBLIC -DH5_BUILT_AS_DYNAMIC_LIB)

# Add the unit tests directory
add_subdirectory(test)

# Auto-generate exports header
target_include_directories(
  Numerical PUBLIC $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/Framework/Numerical> $<INSTALL_INTERFACE:include/Mantid>
)
generate_mantid_export_header(Numerical TRUE)

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
  ${CMAKE_CURRENT_SOURCE_DIR}/inc/MantidNumerical/PocoVersion.h.in
  ${CMAKE_CURRENT_SOURCE_DIR}/inc/MantidNumerical/PocoVersion.h
)

# This section deals with creating the GithubApiHelper implementation Defaults to 40-character string to allocate the
# appropriate amount of space in the final binary for it to be replaced if necessary
set(GITHUB_AUTHORIZATION_TOKEN
    "unsetunsetunsetunsetunsetunsetunsetunset"
    CACHE STRING "token used for authenticated read access to github"
)
configure_file(
  ${CMAKE_CURRENT_SOURCE_DIR}/inc/MantidNumerical/GitHubApiHelper.h.in
  ${CMAKE_CURRENT_SOURCE_DIR}/inc/MantidNumerical/GitHubApiHelper.h
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
set(TARGET_EXPORT_NAME "MantidNumericalTargets")
mtd_install_framework_lib(TARGETS Numerical INSTALL_EXPORT_FILE EXPORT_NAME ${TARGET_EXPORT_NAME})

# location of the properties file
if(NOT DEFINED PROPERTIES_FILE_LOCATION)
  set(PROPERTIES_FILE_LOCATION ${CMAKE_CURRENT_BINARY_DIR}/Mantid.properties.config)
endif()
