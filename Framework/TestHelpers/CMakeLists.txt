set(SRC_FILES
    src/BinaryOperationMDTestHelper.cpp
    src/BoxControllerDummyIO.cpp
    src/ComponentCreationHelper.cpp
    src/FakeObjects.cpp
    src/FileComparisonHelper.cpp
    src/FileResource.cpp
    src/FunctionCreationHelper.cpp
    src/IndirectFitDataCreationHelper.cpp
    src/InstrumentCreationHelper.cpp
    src/JSONGeometryParserTestHelper.cpp
    src/LoggingCleaner.cpp
    src/MDAlgorithmsTestHelper.cpp
    src/MDEventsTestHelper.cpp
    src/MultiDomainFunctionHelper.cpp
    src/MuonGroupingXMLHelper.cpp
    src/MuonWorkspaceCreationHelper.cpp
    src/NexusGeometryTestHelpers.cpp
    src/NexusTestHelper.cpp
    src/ONCatHelper.cpp
    src/ReflectometryHelper.cpp
    src/SANSInstrumentCreationHelper.cpp
    src/ScopedFileHelper.cpp
    src/SingleCrystalDiffractionTestHelper.cpp
    src/StartFrameworkManager.cpp
    src/TearDownWorld.cpp
    src/WorkspaceCreationHelper.cpp
)

set(INC_FILES
    inc/MantidFrameworkTestHelpers/BinaryOperationMDTestHelper.h
    inc/MantidFrameworkTestHelpers/BoxControllerDummyIO.h
    inc/MantidFrameworkTestHelpers/ComponentCreationHelper.h
    inc/MantidFrameworkTestHelpers/FacilityHelper.h
    inc/MantidFrameworkTestHelpers/FakeObjects.h
    inc/MantidFrameworkTestHelpers/FileComparisonHelper.h
    inc/MantidFrameworkTestHelpers/FileResource.h
    inc/MantidFrameworkTestHelpers/FunctionCreationHelper.h
    inc/MantidFrameworkTestHelpers/IndirectFitDataCreationHelper.h
    inc/MantidFrameworkTestHelpers/InstrumentCreationHelper.h
    inc/MantidFrameworkTestHelpers/JSONGeometryParserTestHelper.h
    inc/MantidFrameworkTestHelpers/MDAlgorithmsTestHelper.h
    inc/MantidFrameworkTestHelpers/MDEventsTestHelper.h
    inc/MantidFrameworkTestHelpers/MockAlgorithm.h
    inc/MantidFrameworkTestHelpers/MultiDomainFunctionHelper.h
    inc/MantidFrameworkTestHelpers/MuonGroupingXMLHelper.h
    inc/MantidFrameworkTestHelpers/MuonWorkspaceCreationHelper.h
    inc/MantidFrameworkTestHelpers/NexusFileReader.h
    inc/MantidFrameworkTestHelpers/NexusGeometryTestHelpers.h
    inc/MantidFrameworkTestHelpers/NexusTestHelper.h
    inc/MantidFrameworkTestHelpers/ONCatHelper.h
    inc/MantidFrameworkTestHelpers/ReflectometryHelper.h
    inc/MantidFrameworkTestHelpers/SANSInstrumentCreationHelper.h
    inc/MantidFrameworkTestHelpers/ScopedFileHelper.h
    inc/MantidFrameworkTestHelpers/SingleCrystalDiffractionTestHelper.h
    inc/MantidFrameworkTestHelpers/TearDownWorld.h
    inc/MantidFrameworkTestHelpers/WorkspaceCreationHelper.h
)

if(COVERAGE)
  foreach(loop_var ${INC_FILES})
    set_property(GLOBAL APPEND PROPERTY COVERAGE_SRCS "${CMAKE_CURRENT_SOURCE_DIR}/${loop_var}")
  endforeach(loop_var)
endif()

# We use an object lib to avoid the problems with static libs and dynamic factories. As the linker will only pull in the
# static init if the symbol is used in the executable. A DLL could be used but it requires each helper to be labelled
# with DLL export and complexity one CMake file < DLL exports in every test helper header
add_library(FrameworkTestHelpers OBJECT ${SRC_FILES} ${INC_FILES})
add_library(FrameWorkTestHelpersHeaders INTERFACE)

add_library(Mantid::FrameworkTestHelpers ALIAS FrameworkTestHelpers)
add_library(Mantid::FrameWorkTestHelpersHeaders ALIAS FrameWorkTestHelpersHeaders)
set_target_properties(FrameworkTestHelpers PROPERTIES EXCLUDE_FROM_ALL TRUE FOLDER "UnitTests")

# Unfortunately the INTERFACE property does not propegate through
target_include_directories(FrameworkTestHelpers PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/inc>)

target_include_directories(FrameWorkTestHelpersHeaders INTERFACE $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/inc>)

target_link_libraries(
  FrameworkTestHelpers
  PRIVATE Mantid::API
          Boost::boost
          Mantid::DataObjects
          Mantid::Catalog
          Mantid::DataHandling
          Mantid::Geometry
          Mantid::Kernel
          Mantid::MDAlgorithms
          Mantid::Muon
          Mantid::NexusGeometry
          gmock
)

target_include_directories(FrameworkTestHelpers PRIVATE ${CXXTEST_INCLUDE_DIR})
