# ##############################################################################
# Set the SYSTEM_PACKAGE_TARGET to RUNTIME as we only want to package dlls
# ##############################################################################
set(SYSTEM_PACKAGE_TARGET RUNTIME)
# Also include MSVC runtime libraries when running install commands
set(CMAKE_INSTALL_OPENMP_LIBRARIES TRUE)
include(InstallRequiredSystemLibraries)

# ##############################################################################
# Compiler options.
# ##############################################################################
add_definitions(-D_WINDOWS -DMS_VISUAL_STUDIO)
add_definitions(-D_USE_MATH_DEFINES -DNOMINMAX)
add_definitions(-DGSL_DLL -DJSON_DLL)
add_definitions(-DPOCO_DLL -DPOCO_NO_UNWINDOWS -DPOCO_NO_AUTOMATIC_LIBS)
add_definitions(-DBOOST_CONFIG_SUPPRESS_OUTDATED_MESSAGE)
add_definitions(-D_SCL_SECURE_NO_WARNINGS -D_CRT_SECURE_NO_WARNINGS)
# Prevent deprecation errors from std::tr1 in googletest until it is fixed
# upstream. In MSVC 2017 and later
add_definitions(-D_SILENCE_TR1_NAMESPACE_DEPRECATION_WARNING)
# Suppress warnings about std::iterator as a base. TBB emits this warning
# and it is not yet fixed.
add_definitions(-D_SILENCE_CXX17_ITERATOR_BASE_CLASS_DEPRECATION_WARNING)
add_definitions(-D_SILENCE_CXX17_SHARED_PTR_UNIQUE_DEPRECATION_WARNING)

# ##############################################################################
# Additional compiler flags
# ##############################################################################
# Replace "/" with "\" for use in command prompt
if (NOT CONDA_BUILD)
string(REGEX REPLACE "/" "\\\\" THIRD_PARTY_INCLUDE_DIR
                     "${THIRD_PARTY_DIR}/include/"
)
string(REGEX REPLACE "/" "\\\\" THIRD_PARTY_LIB_DIR "${THIRD_PARTY_DIR}/lib/")
# /MP            - Compile .cpp files in parallel /W3            - Warning Level
# 3 (This is also the default) /external:I    - Ignore third party library
# warnings (similar to "isystem" in GCC)
set(CMAKE_CXX_FLAGS
    "${CMAKE_CXX_FLAGS} /external:I ${THIRD_PARTY_INCLUDE_DIR} /external:I ${THIRD_PARTY_LIB_DIR}\\python${Python_VERSION_MAJOR}.${Python_VERSION_MINOR}\\ "
)
endif()

set(CMAKE_CXX_FLAGS
    "${CMAKE_CXX_FLAGS} \
  /MP /W3 \
  /experimental:external /external:W0 "
)

# Set PCH heap limit, the default does not work when running msbuild from the
# commandline for some reason Any other value lower or higher seems to work but
# not the default. It is fine without this when compiling in the GUI though...
set(VISUALSTUDIO_COMPILERHEAPLIMIT 160)
# It may or may not already be set, so override it if it is (assumes if in CXX
# also in C)
if(CMAKE_CXX_FLAGS MATCHES "(/Zm)([0-9]+)")
  string(REGEX REPLACE "(/Zm)([0-9]+)" "\\1${VISUALSTUDIO_COMPILERHEAPLIMIT}"
                       CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS}
  )
  string(REGEX REPLACE "(/Zm)([0-9]+)" "\\1${VISUALSTUDIO_COMPILERHEAPLIMIT}"
                       CMAKE_C_FLAGS ${CMAKE_C_FLAGS}
  )
else()
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /Zm${VISUALSTUDIO_COMPILERHEAPLIMIT}")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /Zm${VISUALSTUDIO_COMPILERHEAPLIMIT}")
endif()

# HDF5 uses threads::threads target
find_package (Threads)

# ##############################################################################
# Qt5 is always in the same place
# ##############################################################################
if (NOT CONDA_BUILD)
set(Qt5_DIR ${THIRD_PARTY_DIR}/lib/qt5/lib/cmake/Qt5)
endif()


option(CONSOLE "Switch for enabling/disabling the console" ON)

# ##############################################################################
# Windows import library needs to go to bin as well
# ##############################################################################
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/bin)

# ##############################################################################
# Configure IDE/commandline startup scripts
# ##############################################################################
set(WINDOWS_BUILDCONFIG ${PROJECT_SOURCE_DIR}/buildconfig/windows)
if (NOT CONDA_BUILD)
configure_file(
  ${WINDOWS_BUILDCONFIG}/thirdpartypaths.bat.in
  ${PROJECT_BINARY_DIR}/thirdpartypaths.bat @ONLY
)
else()
set(CONDA_BASE_DIR $ENV{CONDA_PREFIX})
configure_file(
  ${WINDOWS_BUILDCONFIG}/thirdpartypaths_conda.bat.in
  ${PROJECT_BINARY_DIR}/thirdpartypaths.bat @ONLY
)
endif()

if(MSVC_VERSION LESS 1911)
  get_filename_component(
    MSVC_VAR_LOCATION "$ENV{VS140COMNTOOLS}/../../VC/" ABSOLUTE
  )
  get_filename_component(
    MSVC_IDE_LOCATION "$ENV{VS140COMNTOOLS}/../IDE" ABSOLUTE
  )
else()
  get_filename_component(MSVC_IDE_LOCATION "${CMAKE_CXX_COMPILER}" DIRECTORY)
  get_filename_component(
    MSVC_IDE_LOCATION "${MSVC_IDE_LOCATION}/../../../../../../.." ABSOLUTE
  )
  set(MSVC_VAR_LOCATION "${MSVC_IDE_LOCATION}/VC/Auxiliary/Build")
  set(MSVC_IDE_LOCATION "${MSVC_IDE_LOCATION}/Common7/IDE")
endif()

# Setup debugger environment to launch in VS without setting paths
if (NOT CONDA_BUILD)
  set(MSVC_PATHS "${THIRD_PARTY_DIR}/bin$<SEMICOLON>${THIRD_PARTY_DIR}/lib/qt5/bin$<SEMICOLON>%PATH%")
else()
  set(MSVC_PATHS "$ENV{CONDA_PREFIX}/Library/bin$<SEMICOLON>$ENV{CONDA_PREFIX}/Library/lib$<SEMICOLON>%PATH%")
endif()

file(TO_CMAKE_PATH ${MSVC_PATHS} MSVC_PATHS)

set(MSVC_BIN_DIR ${PROJECT_BINARY_DIR}/bin/$<CONFIG>)
set(MSVC_IDE_ENV "\
PYTHONPATH=${MSVC_BIN_DIR}\n\
PYTHONHOME=${MSVC_PYTHON_EXECUTABLE_DIR}\n\
PATH=${MSVC_PATHS}")

configure_file(
  ${WINDOWS_BUILDCONFIG}/command-prompt.bat.in
  ${PROJECT_BINARY_DIR}/command-prompt.bat @ONLY
)
configure_file(
  ${WINDOWS_BUILDCONFIG}/pycharm.env.in ${PROJECT_BINARY_DIR}/pycharm.env @ONLY
)

# The IDE may not be installed as we could be just using the build tools
if(EXISTS ${MSVC_IDE_LOCATION}/devenv.exe)
  if (CONDA_BUILD)
  configure_file(
    ${WINDOWS_BUILDCONFIG}/visual-studio_conda.bat.in
    ${PROJECT_BINARY_DIR}/visual-studio.bat @ONLY
  )
  else()
  configure_file(
    ${WINDOWS_BUILDCONFIG}/visual-studio.bat.in
    ${PROJECT_BINARY_DIR}/visual-studio.bat @ONLY
  )
  endif()
endif()
if (CONDA_BUILD)
configure_file(
  ${WINDOWS_BUILDCONFIG}/pycharm_conda.bat.in ${PROJECT_BINARY_DIR}/pycharm.bat @ONLY
)
else()
configure_file(
  ${WINDOWS_BUILDCONFIG}/pycharm.bat.in ${PROJECT_BINARY_DIR}/pycharm.bat @ONLY
)
endif()

# ##############################################################################
# Configure Mantid startup scripts
# ##############################################################################
set(PACKAGING_DIR ${PROJECT_SOURCE_DIR}/buildconfig/CMake/Packaging)
# build version
set(MANTIDPYTHON_PREAMBLE
    "set PYTHONHOME=${MSVC_PYTHON_EXECUTABLE_DIR}\nset PATH=%_BIN_DIR%;%PATH%"
)

configure_file(
  ${PACKAGING_DIR}/mantidpython.bat.in
  ${PROJECT_BINARY_DIR}/mantidpython.bat.in @ONLY
)
# place it in the appropriate directory
file(
  GENERATE
  OUTPUT ${MSVC_BIN_DIR}/mantidpython.bat
  INPUT ${PROJECT_BINARY_DIR}/mantidpython.bat.in
)
# install version
set(MANTIDPYTHON_PREAMBLE
    "set PYTHONHOME=%_BIN_DIR%\nset PATH=%_BIN_DIR%;%_BIN_DIR%\\..\\plugins;%PATH%"
)

configure_file(
  ${PACKAGING_DIR}/mantidpython.bat.in
  ${PROJECT_BINARY_DIR}/mantidpython.bat.install @ONLY
)

# ##############################################################################
# Custom targets to fix-up and run Python entry point code
# ##############################################################################

add_custom_target(SystemTests)
if (NOT STANDALONE_FRAMEWORK)
add_dependencies(SystemTests Framework)
endif()
add_dependencies(SystemTests StandardTestData SystemTestData)
set_target_properties(
  SystemTests
  PROPERTIES
    VS_DEBUGGER_COMMAND
    "${Python_EXECUTABLE}"
    VS_DEBUGGER_COMMAND_ARGUMENTS
    "${CMAKE_SOURCE_DIR}/Testing/SystemTests/scripts/runSystemTests.py --executable \"${MSVC_BIN_DIR}/mantidpython.bat\" --exec-args \" --classic\""
    VS_DEBUGGER_ENVIRONMENT
    "${MSVC_IDE_ENV}"
)
# ##############################################################################
# (Fake) installation variables to keep windows sweet
# ##############################################################################
set(BIN_DIR bin)
set(LIB_DIR ${BIN_DIR})
set(SITE_PACKAGES ${LIB_DIR})
# This is the root of the plugins directory
set(PLUGINS_DIR plugins)

set(WORKBENCH_BIN_DIR ${BIN_DIR})
set(WORKBENCH_LIB_DIR ${LIB_DIR})
set(WORKBENCH_SITE_PACKAGES ${LIB_DIR} CACHE PATH "Path to workbench site packages install location")
set(WORKBENCH_PLUGINS_DIR ${PLUGINS_DIR})
