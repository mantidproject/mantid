# Include useful utils
include(MantidUtils)
include(GenerateMantidExportHeader)
# Make the default build type Release
if(NOT CMAKE_CONFIGURATION_TYPES)
  if(NOT CMAKE_BUILD_TYPE)
    message(STATUS "No build type selected, default to Release.")
    set(CMAKE_BUILD_TYPE
        Release
        CACHE STRING "Choose the type of build." FORCE
    )
    # Set the possible values of build type for cmake-gui
    set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "Debug" "Release" "MinSizeRel" "RelWithDebInfo")
  else()
    message(STATUS "Build type is " ${CMAKE_BUILD_TYPE})
  endif()
endif()

option(ENABLE_OPENGL "Enable OpenGLbased rendering" ON)
option(ENABLE_OPENCASCADE "Enable OpenCascade-based 3D visualisation" ON)
option(USE_PYTHON_DYNAMIC_LIB "Dynamic link python libs" ON)

add_custom_target(check COMMAND ${CMAKE_CTEST_COMMAND})
make_directory(${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/Testing)
message(STATUS "Added target ('check') for unit tests")

# Configure a variable to hold the required test timeout value for all tests
set(TESTING_TIMEOUT
    1200
    CACHE STRING "Timeout in seconds for each test (default 1200=20minutes)"
)

# ######################################################################################################################
# Calculate version number
# ######################################################################################################################
include(VersionNumber)

# if we are building the framework or mantidqt we need these
if(BUILD_MANTIDFRAMEWORK OR BUILD_MANTIDQT)
  find_package(CxxTest)
  if(NOT CXXTEST_FOUND)
    message(STATUS "Could NOT find CxxTest - unit testing not available")
  endif()

  # Avoid the linker failing by including GTest before marking all libs as shared and before we set our warning flags in
  # GNUSetup
  include(GoogleTest)
  include(PyUnitTest)
  enable_testing()

  # We want shared libraries everywhere
  set(BUILD_SHARED_LIBS On)

  # This allows us to group targets logically in Visual Studio
  set_property(GLOBAL PROPERTY USE_FOLDERS ON)

  # Look for stdint and add define if found
  include(CheckIncludeFiles)
  check_include_files(stdint.h stdint)
  if(stdint)
    add_definitions(-DHAVE_STDINT_H)
  endif(stdint)

  # ####################################################################################################################
  # Look for dependencies Do NOT add include_directories commands here. They will affect every target.
  # ####################################################################################################################
  set(Boost_NO_BOOST_CMAKE TRUE)
  # Unless specified see if the boost169 package is installed
  if(EXISTS /usr/lib64/boost169 AND NOT (BOOST_LIBRARYDIR OR BOOST_INCLUDEDIR))
    message(STATUS "Using boost169 package in /usr/lib64/boost169")
    set(BOOST_INCLUDEDIR /usr/include/boost169)
    set(BOOST_LIBRARYDIR /usr/lib64/boost169)
  endif()

  set(Boost_VERBOSE "ON")
  find_package(Boost CONFIG REQUIRED COMPONENTS date_time regex serialization filesystem system)
  add_definitions(-DBOOST_ALL_DYN_LINK -DBOOST_ALL_NO_LIB -DBOOST_BIND_GLOBAL_PLACEHOLDERS)
  # Need this defined globally for our log time values
  add_definitions(-DBOOST_DATE_TIME_POSIX_TIME_STD_CONFIG)
  # Silence issues with deprecated allocator methods in boost regex
  add_definitions(-D_SILENCE_CXX17_OLD_ALLOCATOR_MEMBERS_DEPRECATION_WARNING)

  find_package(Poco 1.4.6 REQUIRED)
  add_definitions(-DPOCO_ENABLE_CPP11)
  find_package(TBB REQUIRED)
  find_package(OpenSSL REQUIRED)
endif()

# if we are building the framework we will need these libraries.
if(BUILD_MANTIDFRAMEWORK)
  find_package(GSL REQUIRED)
  find_package(Nexus 4.3.1 REQUIRED)
  find_package(MuParser REQUIRED)
  find_package(JsonCPP 0.7.0 REQUIRED)
  find_package(Eigen3 3.4 REQUIRED)

  if(ENABLE_OPENCASCADE)
    find_package(OpenCascade REQUIRED)
    add_definitions(-DENABLE_OPENCASCADE)
  endif()

  find_package(
    HDF5 MODULE
    COMPONENTS C CXX HL
    REQUIRED
  )
  set(HDF5_LIBRARIES hdf5::hdf5_cpp hdf5::hdf5)
  set(HDF5_HL_LIBRARIES hdf5::hdf5_hl)
endif()

include(Span)

if(ENABLE_WORKBENCH)
  include(PyUnitTest)
  enable_testing()
endif()

find_package(Doxygen) # optional

# ######################################################################################################################
# Look for OpenMP
# ######################################################################################################################
find_package(OpenMP COMPONENTS CXX)
if(OpenMP_CXX_FOUND)
  link_libraries(OpenMP::OpenMP_CXX)
endif()

# ######################################################################################################################
# Set the c++ standard to 17 - cmake should do the right thing with msvc
# ######################################################################################################################
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ######################################################################################################################
# Setup ccache
# ######################################################################################################################
include(CCacheSetup)

# ######################################################################################################################
# Add compiler options if using gcc
# ######################################################################################################################
if(CMAKE_COMPILER_IS_GNUCXX)
  include(GNUSetup)
elseif(${CMAKE_CXX_COMPILER_ID} MATCHES "Clang")
  include(GNUSetup)
endif()

# ######################################################################################################################
# Configure clang-tidy if the tool is found
# ######################################################################################################################
include(ClangTidy)

# ######################################################################################################################
# Setup cppcheck
# ######################################################################################################################
include(CppCheckSetup)

# ######################################################################################################################
# Setup pylint
# ######################################################################################################################
include(PylintSetup)

# ######################################################################################################################
# Set up the unit tests target
# ######################################################################################################################

# ######################################################################################################################
# External Data for testing
# ######################################################################################################################
if(ENABLE_DOCS
   OR CXXTEST_FOUND
   OR PYUNITTEST_FOUND
)
  include(SetupDataTargets)
endif()

# ######################################################################################################################
# Visibility Setting
# ######################################################################################################################
if(CMAKE_COMPILER_IS_GNUCXX)
  set(CMAKE_CXX_VISIBILITY_PRESET
      hidden
      CACHE STRING ""
  )
endif()

# ######################################################################################################################
# Bundles setting used for install commands if not set by something else e.g. Darwin
# ######################################################################################################################
if(NOT BUNDLES)
  set(BUNDLES "./")
endif()

# ######################################################################################################################
# Set an auto generate warning for .in files.
# ######################################################################################################################
set(AUTO_GENERATE_WARNING "/********** PLEASE NOTE! THIS FILE WAS AUTO-GENERATED FROM CMAKE.  ***********************/")

# ######################################################################################################################
# Enable CXX17_REMOVED_UNARY_BINARY_FUNCTION for boost on osx
# ######################################################################################################################
if(APPLE)
  add_definitions(-D_LIBCPP_ENABLE_CXX17_REMOVED_UNARY_BINARY_FUNCTION)
endif()

# ######################################################################################################################
# Setup pre-commit here as otherwise it will be overwritten by earlier pre-commit hooks being added
# ######################################################################################################################
option(ENABLE_PRECOMMIT "Enable pre-commit framework" ON)
if(ENABLE_PRECOMMIT)
  # Windows should use downloaded ThirdParty version of pre-commit.cmd Everybody else should find one in their PATH
  find_program(
    PRE_COMMIT_EXE
    NAMES pre-commit
    HINTS ~/.local/bin/
  )
  if(NOT PRE_COMMIT_EXE)
    message(FATAL_ERROR "Failed to find pre-commit see https://developer.mantidproject.org/GettingStarted.html")
  endif()

  if(WIN32)
    execute_process(
      COMMAND "${PRE_COMMIT_EXE}" install --overwrite
      WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
      RESULT_VARIABLE PRE_COMMIT_RESULT
    )
    if(NOT PRE_COMMIT_RESULT EQUAL "0")
      message(FATAL_ERROR "Pre-commit install failed with ${PRE_COMMIT_RESULT}")
    endif()
  else() # linux and osx
    execute_process(
      COMMAND bash -c "${PRE_COMMIT_EXE} install"
      WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
      RESULT_VARIABLE STATUS
    )
    if(STATUS AND NOT STATUS EQUAL 0)
      message(
        FATAL_ERROR
          "Pre-commit tried to install itself into your repository, but failed to do so. Is it installed on your system?"
      )
    endif()
  endif()
endif()

# ######################################################################################################################
# Set a flag to indicate that this script has been called
# ######################################################################################################################

set(COMMONSETUP_DONE TRUE)
