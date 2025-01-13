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
add_subdirectory(NexusCpp)
add_subdirectory(Kernel)
add_subdirectory(Parallel)
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
