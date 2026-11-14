# Look for dependencies - bail out if any necessary ones not found

check_include_files(stdint.h stdint)
if(stdint)
  add_definitions(-DHAVE_STDINT_H)
endif()

add_custom_target(FrameworkTests) # target for all framework tests
include(TestTargetFunctions)
add_dependencies(check FrameworkTests)
add_dependencies(FrameworkTests Framework)

add_subdirectory(Types)
add_subdirectory(Json)
add_subdirectory(LegacyNexus)
add_subdirectory(Kernel)
# HistogramData has header-only dependency on Kernel, so Kernel comes after.
add_subdirectory(Algorithms)
add_subdirectory(API)
add_subdirectory(Beamline)
add_subdirectory(Catalog)
add_subdirectory(Crystal)
add_subdirectory(CurveFitting)
add_subdirectory(DataHandling)
add_subdirectory(DataObjects)
add_subdirectory(Geometry)
add_subdirectory(HistogramData)
add_subdirectory(ICat)
add_subdirectory(Indexing)
add_subdirectory(LiveData)
add_subdirectory(Muon)
add_subdirectory(Nexus)
add_subdirectory(NexusGeometry)
add_subdirectory(PythonInterface)
add_subdirectory(Reflectometry)
add_subdirectory(SINQ)
add_subdirectory(TestHelpers)
add_subdirectory(WorkflowAlgorithms)
add_subdirectory(MDAlgorithms)
add_subdirectory(Doxygen)
add_subdirectory(ScriptRepository)

# Build with MPI (Linux only)
if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
  message(STATUS "CMAKE_SYSTEM_NAME=${CMAKE_SYSTEM_NAME}; MPI_BUILD=${MPI_BUILD}")
  add_subdirectory(MPIAlgorithms)
endif()

# Add a custom target to build all of the Framework

set(FRAMEWORK_LIBS
    Kernel
    Json
    HistogramData
    Indexing
    Beamline
    Geometry
    API
    DataObjects
    PythonInterface
    DataHandling
    Nexus
    NexusGeometry
    Algorithms
    CurveFitting
    ICat
    Catalog
    Crystal
    MDAlgorithms
    WorkflowAlgorithms
    LiveData
    SINQ
    Muon
    Reflectometry
)

add_custom_target(Framework DEPENDS ${FRAMEWORK_LIBS})

# ######################################################################################################################
# mantid.simpleapi type stub
# ######################################################################################################################
# Regenerate the committed PythonInterface/mantid/simpleapi.pyi with `ninja simpleapi_stubs`. It is not part of "all"
# because it rewrites a file in the source tree. Note that `clean` deletes the stub; restore it with git or rerun the
# target. Where the MPIAlgorithms plugin is built (Linux), the target also writes the uncommitted _simpleapi_mpi.pyi for
# its algorithms. Package builds pass --no-format here, because formatting needs prek or pre-commit.
set(SIMPLEAPI_STUBS_ARGS
    ""
    CACHE STRING "Extra arguments for generate_simpleapi_stubs.py, e.g. --no-format"
)
set(_simpleapi_stub ${CMAKE_CURRENT_SOURCE_DIR}/PythonInterface/mantid/simpleapi.pyi)
set(_simpleapi_stub_generator ${CMAKE_SOURCE_DIR}/tools/python_typing/generate_simpleapi_stubs.py)
# Python algorithms and the mantid package are read from the source tree
file(GLOB_RECURSE _simpleapi_stub_python_sources ${CMAKE_CURRENT_SOURCE_DIR}/PythonInterface/mantid/*.py
     ${CMAKE_CURRENT_SOURCE_DIR}/PythonInterface/plugins/*.py
)
if(CMAKE_GENERATOR MATCHES "Visual Studio" OR CMAKE_GENERATOR MATCHES "Xcode")
  set(_simpleapi_stub_module_dir ${CMAKE_BINARY_DIR}/bin/$<CONFIG>)
else()
  set(_simpleapi_stub_module_dir ${CMAKE_BINARY_DIR}/bin)
endif()
# Depending on the library targets (rather than the Framework utility target, which only orders the build) re-runs the
# generator whenever one is relinked. The Python extension modules are the dependencies of PythonInterface.
get_target_property(_simpleapi_stub_python_modules PythonInterface MANUALLY_ADDED_DEPENDENCIES)
# An empty user properties directory (HOME on Linux/macOS, APPDATA on Windows) keeps settings such as default.facility
# out of the stub
set(_simpleapi_stub_home ${CMAKE_CURRENT_BINARY_DIR}/simpleapi_stubs_home)
set(_simpleapi_stub_command
    ${CMAKE_COMMAND} -E env "PYTHONPATH=${_simpleapi_stub_module_dir}" "HOME=${_simpleapi_stub_home}"
    "APPDATA=${_simpleapi_stub_home}" ${Python_EXECUTABLE} ${_simpleapi_stub_generator} ${SIMPLEAPI_STUBS_ARGS}
)
set(_simpleapi_stub_outputs ${_simpleapi_stub})
set(_simpleapi_mpi_stub_commands)
if(TARGET MPIAlgorithms)
  list(APPEND _simpleapi_stub_python_modules MPIAlgorithms)
  list(APPEND _simpleapi_stub_outputs ${CMAKE_CURRENT_SOURCE_DIR}/PythonInterface/mantid/_simpleapi_mpi.pyi)
  set(_simpleapi_mpi_stub_commands COMMAND ${_simpleapi_stub_command} --mpi-only)
endif()

add_custom_command(
  OUTPUT ${_simpleapi_stub_outputs}
  COMMAND ${CMAKE_COMMAND} -E make_directory ${_simpleapi_stub_home}
  COMMAND ${_simpleapi_stub_command} --all ${_simpleapi_mpi_stub_commands}
  DEPENDS ${FRAMEWORK_LIBS} ${_simpleapi_stub_python_modules} ${_simpleapi_stub_generator}
          ${_simpleapi_stub_python_sources}
  WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
  VERBATIM
)
add_custom_target(simpleapi_stubs DEPENDS ${_simpleapi_stub})
set_property(TARGET simpleapi_stubs PROPERTY FOLDER "MantidFramework/Python")

# Installation settings These need to be here so that a Framework only install will pick them up
#
# N.B. INBUNDLE variable is empty except on Mac (set in DarwinSetup.cmake)

# Create instrument directory
foreach(_bundle ${BUNDLES})
  install(
    DIRECTORY ../instrument/
    DESTINATION ${_bundle}/instrument
    COMPONENT Runtime
    PATTERN "*UNIT_TESTING*" EXCLUDE
    PATTERN ".gitignore" EXCLUDE
  )
  # Ships .py files but only ship compiled pyd files for supported platforms.
  if(WIN32) # General windows environment
    if(CMAKE_SIZEOF_VOID_P EQUAL 8) # Recommended way of detecting 64- vs 32-bit build Excludes .so files & _win32
                                    # binaries
      install(
        DIRECTORY ../scripts/
        DESTINATION ${_bundle}/scripts
        COMPONENT Runtime
        PATTERN "*.pyc" EXCLUDE
        PATTERN ".svn" EXCLUDE
        PATTERN ".gitignore" EXCLUDE
        PATTERN "*.so" EXCLUDE
        PATTERN "*_win32.pyd" EXCLUDE
        PATTERN "CMakeLists.txt" EXCLUDE
        PATTERN "test" EXCLUDE
      )
    else()
      # Excludes so files & _win64 binaries
      install(
        DIRECTORY ../scripts/
        DESTINATION ${_bundle}scripts
        COMPONENT Runtime
        PATTERN "*.pyc" EXCLUDE
        PATTERN ".svn" EXCLUDE
        PATTERN ".gitignore" EXCLUDE
        PATTERN "*.so" EXCLUDE
        PATTERN "*_win64.pyd" EXCLUDE
        PATTERN "CMakeLists.txt" EXCLUDE
        PATTERN "test" EXCLUDE
      )
    endif()

    # Also ship mingw libraries for Inelastic fortran code. We need to do a better job here and build things
    file(GLOB MINGW_DLLS "${THIRD_PARTY_DIR}/bin/mingw/*.dll")
    install(FILES ${MINGW_DLLS} DESTINATION ${BIN_DIR})
  else()
    install(
      DIRECTORY ../scripts/
      DESTINATION ${_bundle}scripts
      COMPONENT Runtime
      PATTERN "*.pyc" EXCLUDE
      PATTERN ".svn" EXCLUDE
      PATTERN ".gitignore" EXCLUDE
      PATTERN "*_win*.pyd" EXCLUDE
      PATTERN "*_lnx64.so" EXCLUDE
      PATTERN "CMakeLists.txt" EXCLUDE
      PATTERN "test" EXCLUDE
    )
  endif()
endforeach()

# THIS MUST BE THE LAST SUB_DIRECTORY ADDED. See Properties/CMakeLists.txt. This is included by the top-level CMakeLists
# if it is a full build but do it here for a Framework only
if(NOT FULL_PACKAGE_BUILD)
  add_subdirectory(PostInstall)
endif()

include(CMakePackageConfigHelpers)
configure_package_config_file(
  ${CMAKE_SOURCE_DIR}/buildconfig/CMake/MantidFrameworkConfig.cmake.in
  ${CMAKE_CURRENT_BINARY_DIR}/MantidFrameworkConfig.cmake
  INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/MantidFramework
)

write_basic_package_version_file(
  ${CMAKE_CURRENT_BINARY_DIR}/MantidFrameworkConfigVersion.cmake
  VERSION ${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_PATCH}${VERSION_TWEAK}
  COMPATIBILITY SameMajorVersion
)

install(
  FILES "${CMAKE_CURRENT_BINARY_DIR}/MantidFrameworkConfig.cmake"
        "${CMAKE_CURRENT_BINARY_DIR}/MantidFrameworkConfigVersion.cmake"
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/MantidFramework
  COMPONENT Devel
)

install(
  FILES ${CMAKE_SOURCE_DIR}/buildconfig/CMake/FindJsonCPP.cmake ${CMAKE_SOURCE_DIR}/buildconfig/CMake/FindMuParser.cmake
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/MantidFramework/find_modules
  COMPONENT Devel
)
