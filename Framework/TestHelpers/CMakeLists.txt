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
    src/ParallelRunner.cpp
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
    inc/MantidFrameworkTestHelpers/MultiDomainFunctionHelper.h
    inc/MantidFrameworkTestHelpers/MuonGroupingXMLHelper.h
    inc/MantidFrameworkTestHelpers/MuonWorkspaceCreationHelper.h
    inc/MantidFrameworkTestHelpers/NexusFileReader.h
    inc/MantidFrameworkTestHelpers/NexusGeometryTestHelpers.h
    inc/MantidFrameworkTestHelpers/NexusTestHelper.h
    inc/MantidFrameworkTestHelpers/ONCatHelper.h
    inc/MantidFrameworkTestHelpers/ParallelAlgorithmCreation.h
    inc/MantidFrameworkTestHelpers/ParallelRunner.h
    inc/MantidFrameworkTestHelpers/ReflectometryHelper.h
    inc/MantidFrameworkTestHelpers/SANSInstrumentCreationHelper.h
    inc/MantidFrameworkTestHelpers/ScopedFileHelper.h
    inc/MantidFrameworkTestHelpers/SingleCrystalDiffractionTestHelper.h
    inc/MantidFrameworkTestHelpers/TearDownWorld.h
    inc/MantidFrameworkTestHelpers/WorkspaceCreationHelper.h
)

if(COVERAGE)
  foreach(loop_var ${INC_FILES})
    set_property(GLOBAL APPEND
                 PROPERTY COVERAGE_SRCS
                          "${CMAKE_CURRENT_SOURCE_DIR}/${loop_var}")
  endforeach(loop_var)
endif()

add_library(FrameworkTestHelpers STATIC ${SRC_FILES} ${INC_FILES})
add_library(Mantid::FrameworkTestHelpers ALIAS FrameworkTestHelpers)
set_property(TARGET FrameworkTestHelpers PROPERTY FOLDER "UnitTests")

target_include_directories(FrameworkTestHelpers PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/inc>
)

target_link_libraries(FrameworkTestHelpers
                        PUBLIC
                        Mantid::API
                        PRIVATE
                        Mantid::DataObjects
                        Mantid::Catalog
                        Mantid::DataHandling
                        Mantid::Geometry
                        Mantid::Kernel
                        Mantid::MDAlgorithms
                        Mantid::Muon
                        Mantid::NexusGeometry
)

target_include_directories(FrameworkTestHelpers PRIVATE
                           ${CXXTEST_INCLUDE_DIR})