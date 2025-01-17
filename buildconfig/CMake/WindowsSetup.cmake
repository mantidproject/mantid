# Include MSVC runtime libraries when running install commands
set(CMAKE_INSTALL_OPENMP_LIBRARIES TRUE)
include(InstallRequiredSystemLibraries)

# ######################################################################################################################
# Compiler options.
# ######################################################################################################################
add_definitions(-D_WINDOWS -DMS_VISUAL_STUDIO)
add_definitions(-D_USE_MATH_DEFINES -DNOMINMAX)
add_definitions(-DGSL_DLL -DJSON_DLL)
add_definitions(-DPOCO_DLL -DPOCO_NO_UNWINDOWS -DPOCO_NO_AUTOMATIC_LIBS)
add_definitions(-DBOOST_CONFIG_SUPPRESS_OUTDATED_MESSAGE)
add_definitions(-D_SCL_SECURE_NO_WARNINGS -D_CRT_SECURE_NO_WARNINGS)
# Prevent deprecation errors from std::tr1 in googletest until it is fixed upstream. In MSVC 2017 and later
add_definitions(-D_SILENCE_TR1_NAMESPACE_DEPRECATION_WARNING)
# Suppress codecvt deprecation warning when building with Ninja. Using codecvt is still the recommended solution for
# converting from std::string to std::wstring. See https://stackoverflow.com/a/18597384
add_definitions(-D_SILENCE_CXX17_CODECVT_HEADER_DEPRECATION_WARNING)
# Suppress warnings about std::iterator as a base. TBB emits this warning and it is not yet fixed.
add_definitions(-D_SILENCE_CXX17_ITERATOR_BASE_CLASS_DEPRECATION_WARNING)
add_definitions(-D_SILENCE_CXX17_SHARED_PTR_UNIQUE_DEPRECATION_WARNING)

# ######################################################################################################################
# Additional compiler flags
# ######################################################################################################################

# /MP - Compile .cpp files in parallel /W3 - Warning Level 3 (This is also the default) /external:I - Ignore third party
# library  warnings (similar to "isystem" in GCC) /bigobj Compile large test targets by losing compatibility with MSVC
# 2005
set(CMAKE_CXX_FLAGS
    "${CMAKE_CXX_FLAGS} \
  /MP /W3 /bigobj \
  /wd4251 /wd4275 /wd4373 \
  /experimental:external /external:W0 "
)
# the warnings suppressed are:
#
# 4251 'identifier' : class 'type' needs to have dll-interface to be used by clients of class 'type2' Things from the
# std library give these warnings and we can't do anything about them.
#
# 4275 Given that we are compiling everything with msvc under Windows and linking all with the same runtime we can
# disable the warning about inheriting from a non-exported interface, e.g. std::runtime_error
#
# 4373 previous versions of the compiler did not override when parameters only differed by const/volatile qualifiers.
# This is basically saying that it now follows the C++ standard and doesn't seem useful

# Set PCH heap limit, the default does not work when running msbuild from the commandline for some reason Any other
# value lower or higher seems to work but not the default. It is fine without this when compiling in the GUI though...
set(VISUALSTUDIO_COMPILERHEAPLIMIT 160)
# It may or may not already be set, so override it if it is (assumes if in CXX also in C)
if(CMAKE_CXX_FLAGS MATCHES "(/Zm)([0-9]+)")
  string(REGEX REPLACE "(/Zm)([0-9]+)" "\\1${VISUALSTUDIO_COMPILERHEAPLIMIT}" CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS})
  string(REGEX REPLACE "(/Zm)([0-9]+)" "\\1${VISUALSTUDIO_COMPILERHEAPLIMIT}" CMAKE_C_FLAGS ${CMAKE_C_FLAGS})
else()
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /Zm${VISUALSTUDIO_COMPILERHEAPLIMIT}")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /Zm${VISUALSTUDIO_COMPILERHEAPLIMIT}")
endif()

# Define a new configuration for debugging with Conda. Crucially it must link to the MSVC release runtime but we switch
# off all optimzations
set(_conda_debug_cfg_name DebugWithRelRuntime)
string(TOUPPER ${_conda_debug_cfg_name} _conda_debug_cfg_name_upper)
include(CCacheSetup)
if(USE_CCACHE AND CCACHE_FOUND)
  # For a single-config generator like Ninja, aim to use CCache to speed up builds. The Zi flag is not supported by
  # CCache so we need to use Z7 to allow debugging.
  set(_language_flags "/Z7 /Ob0 /Od /RTC1")
else()
  # For a multi-config generator like MSVC, CCache is not used, so the Zi flag can be used for debugging.
  set(_language_flags "/Zi /Ob0 /Od /RTC1")
endif()
# C/CXX flags
foreach(lang C CXX)
  set(CMAKE_${lang}_FLAGS_${_conda_debug_cfg_name_upper}
      ${_language_flags}
      CACHE STRING "" FORCE
  )
endforeach()
# Linker
foreach(t EXE SHARED MODULE)
  set(CMAKE_${t}_LINKER_FLAGS_${_conda_debug_cfg_name_upper}
      ${CMAKE_${t}_LINKER_FLAGS_RELWITHDEBINFO}
      CACHE STRING "" FORCE
  )
endforeach()

# Set configurations. We also dump MinSizeRel & Debug as the former is not used and the latter will not work
set(CMAKE_CONFIGURATION_TYPES
    "${_conda_debug_cfg_name};Release;RelWithDebInfo"
    CACHE STRING "" FORCE
)
message(
  STATUS
    "Detected a build with Conda on Windows. Resetting available build configurations to ${CMAKE_CONFIGURATION_TYPES}"
)

# HDF5 uses threads::threads target
find_package(Threads)

option(CONSOLE "Switch for enabling/disabling the console" ON)

# ######################################################################################################################
# Windows import library needs to go to bin as well
# ######################################################################################################################
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/bin)

# ######################################################################################################################
# Configure IDE/commandline startup scripts
# ######################################################################################################################
set(WINDOWS_BUILDCONFIG ${PROJECT_SOURCE_DIR}/buildconfig/windows)
set(CONDA_BASE_DIR $ENV{CONDA_PREFIX})
configure_file(${WINDOWS_BUILDCONFIG}/thirdpartypaths_conda.bat.in ${PROJECT_BINARY_DIR}/thirdpartypaths.bat @ONLY)

if(MSVC_VERSION LESS 1911)
  get_filename_component(MSVC_VAR_LOCATION "$ENV{VS140COMNTOOLS}/../../VC/" ABSOLUTE)
  get_filename_component(MSVC_IDE_LOCATION "$ENV{VS140COMNTOOLS}/../IDE" ABSOLUTE)
else()
  get_filename_component(MSVC_IDE_LOCATION "${CMAKE_CXX_COMPILER}" DIRECTORY)
  get_filename_component(MSVC_IDE_LOCATION "${MSVC_IDE_LOCATION}/../../../../../../.." ABSOLUTE)
  set(MSVC_VAR_LOCATION "${MSVC_IDE_LOCATION}/VC/Auxiliary/Build")
  set(MSVC_IDE_LOCATION "${MSVC_IDE_LOCATION}/Common7/IDE")
endif()

# Setup debugger environment to launch in VS without setting paths
set(MSVC_PATHS "$ENV{CONDA_PREFIX}/Library/bin$<SEMICOLON>$ENV{CONDA_PREFIX}/Library/lib$<SEMICOLON>%PATH%")

file(TO_CMAKE_PATH ${MSVC_PATHS} MSVC_PATHS)

get_property(_is_multi_config GLOBAL PROPERTY GENERATOR_IS_MULTI_CONFIG)
if(_is_multi_config)
  set(MSVC_BIN_DIR ${PROJECT_BINARY_DIR}/bin/$<CONFIG>)
else()
  set(MSVC_BIN_DIR ${PROJECT_BINARY_DIR}/bin)
endif()

set(MSVC_IDE_ENV
    "\
PYTHONPATH=${MSVC_BIN_DIR}\;${PROJECT_SOURCE_DIR}/qt/applications/workbench\;${PROJECT_SOURCE_DIR}/Framework/PythonInterface\;${PROJECT_SOURCE_DIR}/qt/python/mantidqt\;${PROJECT_SOURCE_DIR}/qt/python/mantidqtinterfaces\n\
PYTHONHOME=${MSVC_PYTHON_EXECUTABLE_DIR}\n\
PATH=${MSVC_PATHS}"
)

configure_file(${WINDOWS_BUILDCONFIG}/command-prompt.bat.in ${PROJECT_BINARY_DIR}/command-prompt.bat @ONLY)

# The IDE may not be installed as we could be just using the build tools
if(EXISTS ${MSVC_IDE_LOCATION}/devenv.exe)
  configure_file(${WINDOWS_BUILDCONFIG}/visual-studio_conda.bat.in ${PROJECT_BINARY_DIR}/visual-studio.bat @ONLY)
  configure_file(
    ${WINDOWS_BUILDCONFIG}/visual-studio_conda_ninja.bat.in ${PROJECT_BINARY_DIR}/visual-studio_ninja.bat @ONLY
  )
endif()

# ######################################################################################################################
# Custom targets to fix-up and run Python entry point code
# ######################################################################################################################

if(MANTID_FRAMEWORK_LIB STREQUAL "BUILD")
  add_custom_target(SystemTests)
  add_dependencies(SystemTests Framework)
  add_dependencies(SystemTests StandardTestData SystemTestData)
  set_target_properties(
    SystemTests
    PROPERTIES VS_DEBUGGER_COMMAND "${Python_EXECUTABLE}"
               VS_DEBUGGER_COMMAND_ARGUMENTS VS_DEBUGGER_ENVIRONMENT
               "${CMAKE_SOURCE_DIR}/Testing/SystemTests/scripts/runSystemTests.py --executable python3"
               "${MSVC_IDE_ENV}"
  )
endif()

# ######################################################################################################################
# (Fake) installation variables to keep windows sweet
# ######################################################################################################################
set(BIN_DIR bin)
set(LIB_DIR ${BIN_DIR})
set(SITE_PACKAGES ${LIB_DIR})
# This is the root of the plugins directory
set(PLUGINS_DIR plugins)

set(WORKBENCH_BIN_DIR ${BIN_DIR})
set(WORKBENCH_LIB_DIR ${LIB_DIR})
set(WORKBENCH_SITE_PACKAGES
    ${LIB_DIR}
    CACHE PATH "Path to workbench site packages install location"
)
set(WORKBENCH_PLUGINS_DIR ${PLUGINS_DIR})
