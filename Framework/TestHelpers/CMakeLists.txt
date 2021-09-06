set(SRC_FILES
    src/BinaryOperationMDTestHelper.cpp
    src/BoxControllerDummyIO.cpp
    src/ComponentCreationHelper.cpp
    src/DataProcessorTestHelper.cpp
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
    inc/MantidTestHelpers/BinaryOperationMDTestHelper.h
    inc/MantidTestHelpers/BoxControllerDummyIO.h
    inc/MantidTestHelpers/ComponentCreationHelper.h
    inc/MantidTestHelpers/DataProcessorTestHelper.h
    inc/MantidTestHelpers/FacilityHelper.h
    inc/MantidTestHelpers/FakeObjects.h
    inc/MantidTestHelpers/FileComparisonHelper.h
    inc/MantidTestHelpers/FileResource.h
    inc/MantidTestHelpers/FunctionCreationHelper.h
    inc/MantidTestHelpers/IndirectFitDataCreationHelper.h
    inc/MantidTestHelpers/InstrumentCreationHelper.h
    inc/MantidTestHelpers/JSONGeometryParserTestHelper.h
    inc/MantidTestHelpers/MDAlgorithmsTestHelper.h
    inc/MantidTestHelpers/MDEventsTestHelper.h
    inc/MantidTestHelpers/MultiDomainFunctionHelper.h
    inc/MantidTestHelpers/MuonGroupingXMLHelper.h
    inc/MantidTestHelpers/MuonWorkspaceCreationHelper.h
    inc/MantidTestHelpers/NexusFileReader.h
    inc/MantidTestHelpers/NexusGeometryTestHelpers.h
    inc/MantidTestHelpers/NexusTestHelper.h
    inc/MantidTestHelpers/ONCatHelper.h
    inc/MantidTestHelpers/ParallelAlgorithmCreation.h
    inc/MantidTestHelpers/ParallelRunner.h
    inc/MantidTestHelpers/ReflectometryHelper.h
    inc/MantidTestHelpers/SANSInstrumentCreationHelper.h
    inc/MantidTestHelpers/ScopedFileHelper.h
    inc/MantidTestHelpers/SingleCrystalDiffractionTestHelper.h
    inc/MantidTestHelpers/TearDownWorld.h
    inc/MantidTestHelpers/WorkspaceCreationHelper.h
)

if(COVERAGE)
  foreach(loop_var ${INC_FILES})
    set_property(GLOBAL APPEND
                 PROPERTY COVERAGE_SRCS
                          "${CMAKE_CURRENT_SOURCE_DIR}/${loop_var}")
  endforeach(loop_var)
endif()

add_library(TestHelpers STATIC ${SRC_FILES} ${INC_FILES})
add_library(Mantid::TestHelpers ALIAS TestHelpers)
set_property(TARGET TestHelpers PROPERTY FOLDER "UnitTests")

target_include_directories(TestHelpers PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/inc>
)

target_link_libraries(TestHelpers
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
                        # TODO these should be split out into
                        # a GUI test helpers target
                        MantidQtWidgetsCommonQt5
                        Qt5::Core
)

target_include_directories( TestHelpers PRIVATE
                            ${CXXTEST_INCLUDE_DIR}

                            )