# Common definitions
add_definitions(-DBOOST_PYTHON_NO_LIB -DNPY_NO_DEPRECATED_API=NPY_1_7_API_VERSION)
if(CMAKE_COMPILER_IS_GNUCXX
   AND Boost_MAJOR_VERSION EQUAL "1"
   AND Boost_MINOR_VERSION GREATER "63"
   AND Boost_MINOR_VERSION LESS "66"
)
  # Several bugs in boost 1.64-1.65 prevent python modules from loading on gcc without this definition:
  # https://github.com/boostorg/python/issues/131
  add_definitions(-DBOOST_PYTHON_USE_GCC_SYMBOL_VISIBILITY)
endif()

# Keep boost python separate from other boost
set(_boost_libs_orig ${Boost_LIBRARIES})
set(Boost_LIBRARIES)
set(Boost_VERBOSE "ON")
find_package(
  Boost CONFIG
  COMPONENTS python${Python_VERSION_MAJOR}${Python_VERSION_MINOR}
  REQUIRED
)
set(BoostPython_LIBRARIES ${Boost_LIBRARIES})
set(Boost_LIBRARIES ${_boost_libs_orig})
unset(_boost_libs_orig)

# First, common Python code
add_subdirectory(core)

# A function for generating the exports - MODULE_TEMPLATE: The file containing the @EXPORT_FUNCTIONS@ and
# @EXPORT_DECLARE@ flags to replace - OUTPUT_FILE:The path to the generated output file - ... The list of export files
function(CREATE_MODULE MODULE_TEMPLATE OUTPUT_FILE)
  set(_fwd_declarations)
  set(_function_calls)
  foreach(_cppfile ${ARGN})
    # pull out all functions named 'void export...'
    file(STRINGS ${_cppfile} _definitions REGEX "( *)?void *export.*().*")
    foreach(_func_definition ${_definitions})
      # create a forward declaration and function call
      string(STRIP "${_func_definition}" _func_definition)
      string(REGEX REPLACE "(void *export.*\\(\\)).*" "\\1" _func_declaration "${_func_definition}")
      # add to list of declarations
      set(_fwd_declarations "${_fwd_declarations}\n${_func_declaration}\;")
      # strip void and add to call list
      string(REGEX REPLACE "void *" "" _func_call "${_func_declaration}")
      set(_function_calls "${_function_calls}\n${_func_call}\;")
    endforeach()
  endforeach()
  string(STRIP "${_fwd_declarations}" _fwd_declarations)
  string(STRIP "${_function_calls}" _function_calls)

  # Configure the final file
  set(EXPORT_DECLARE ${_fwd_declarations})
  set(EXPORT_FUNCTIONS ${_function_calls})

  configure_file(${MODULE_TEMPLATE} ${OUTPUT_FILE})
endfunction()

# A function for setting the correct properties on the individual targets
function(SET_PYTHON_PROPERTIES TARGET TARGET_NAME)
  # No library prefixes
  set_target_properties(${TARGET} PROPERTIES PREFIX "")
  # Library name needs to end in .pyd for Windows
  if(MSVC)
    set_target_properties(${TARGET} PROPERTIES SUFFIX .pyd)
  elseif(APPLE)
    # and in .so on the Mac
    set_target_properties(${TARGET} PROPERTIES SUFFIX .so)
  endif()
  # Set the name
  set_target_properties(${TARGET} PROPERTIES OUTPUT_NAME ${TARGET_NAME})
  # Debug python library expects imported module names to end in _d
  if(PYTHON_DEBUG_LIBRARY)
    set_target_properties(${TARGET} PROPERTIES DEBUG_OUTPUT_NAME ${TARGET_NAME}_d)
  endif()
  # Group within VS
  set_property(TARGET ${TARGET} PROPERTY FOLDER "MantidFramework/Python")
endfunction()

# ######################################################################################################################
# mantid package
# ######################################################################################################################
# Common install RPATH setting for all extension modules
if(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
  set(EXT_INSTALL_RPATH "${DL_ORIGIN_TAG}/../../../MacOS;${DL_ORIGIN_TAG}/../../../Frameworks")
elseif(${CMAKE_SYSTEM_NAME} STREQUAL "Linux")
  set(EXT_INSTALL_RPATH ${DL_ORIGIN_TAG}/../../../${LIB_DIR})
endif()

# All targets can see these includes
include_directories(core/inc)
# TODO: Move all common code to the core library to remove the need for linking submodules together
include_directories(inc)
add_subdirectory(mantid)

include(PythonPackageTargetFunctions)

add_python_package(
  PythonInterface
  EGGLINKNAME mantid
  GENERATE_SITECUSTOMIZE
)

set_property(TARGET PythonInterface PROPERTY FOLDER "MantidFramework/Python")
add_dependencies(
  PythonInterface
  PythonInterfaceCore
  PythonKernelModule
  PythonGeometryModule
  PythonAPIModule
  PythonDataObjectsModule
  PythonCurveFittingModule
)

# Clear any leftover bin/$(Configuration)/mantid/ folder, from when PythonInterface was being copied over. The last
# semicolon is there to have an iteration with an empty string which makes the last check just bin/mantid for
# non-Windows builds
set(_windows_configurations "Debug/;Release/;")
foreach(_configuration IN LISTS _windows_configurations)
  set(_old_mantid_dir ${CMAKE_BINARY_DIR}/bin/${_configuration}mantid)
  if(EXISTS ${_old_mantid_dir})
    message("Removing old mantid PythonInterface directory: ${_old_mantid_dir}")
    file(REMOVE_RECURSE ${_old_mantid_dir})
  endif()
endforeach(_configuration)

# ######################################################################################################################
# Python algorithms
# ######################################################################################################################
clean_orphaned_pyc_files(${CMAKE_CURRENT_SOURCE_DIR}/plugins)

add_subdirectory(plugins)

# tests
set(PYTHONINTERFACE_PLUGINS_DIR ${CMAKE_CURRENT_SOURCE_DIR}/plugins)
add_subdirectory(test)

# Installation settings

# Python algorithms
mtd_install_dirs(
  DIRECTORY
  plugins/
  INSTALL_DIRS
  ${PLUGINS_DIR}/python
  ${WORKBENCH_PLUGINS_DIR}/python
  EXCLUDE
  "*.pyc"
)
