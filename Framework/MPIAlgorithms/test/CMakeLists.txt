if(NOT CXXTEST_FOUND)
  return()
endif()

set(TEST_FILES BroadcastWorkspaceTest.h GatherWorkspacesTest.h)

include_directories(SYSTEM ${CXXTEST_INCLUDE_DIR})
cxxtest_add_test(MPIAlgorithmsTest ${TEST_FILES} ${GMOCK_TEST_FILES})
target_link_libraries(
  MPIAlgorithmsTest
  PUBLIC ${MPI_CXX_LIBRARIES} ${Boost_MPI_LIBRARY} ${Boost_SERIALIZATION_LIBRARY}
  PRIVATE Mantid::MPIAlgorithms
)

if(TARGET GTest::gmock AND TARGET GTest::gtest)
  target_link_libraries(MPIAlgorithmsTest PRIVATE GTest::gmock GTest::gtest)
else()
  target_link_libraries(MPIAlgorithmsTest PRIVATE gmock gtest)
endif()

add_framework_test_helpers(MPIAlgorithmsTest)
add_dependencies(FrameworkTests MPIAlgorithmsTest)

# MPI can only be initialized once per process, so the tests of when it is initialized need a process of their own
cxxtest_add_test(MPILifetimeTest MPILifetimeTest.h)
target_link_libraries(
  MPILifetimeTest
  PUBLIC ${MPI_CXX_LIBRARIES}
  PRIVATE Mantid::MPIAlgorithms
)
if(TARGET GTest::gmock AND TARGET GTest::gtest)
  target_link_libraries(MPILifetimeTest PRIVATE GTest::gmock GTest::gtest)
else()
  target_link_libraries(MPILifetimeTest PRIVATE gmock gtest)
endif()
add_framework_test_helpers(MPILifetimeTest)
add_dependencies(FrameworkTests MPILifetimeTest)
set_property(TARGET MPILifetimeTest PROPERTY FOLDER "UnitTests")
set_property(
  TEST MPILifetimeTest_MPILifetimeTest
  APPEND
  PROPERTY LABELS "MPI"
)

set_property(TARGET MPIAlgorithmsTest PROPERTY FOLDER "UnitTests")

if(MPIEXEC_EXECUTABLE)

  add_test(NAME MPIAlgorithmsTest COMMAND ${MPIEXEC_EXECUTABLE} ${MPIEXEC_NUMPROC_FLAG} 1
                                          $<TARGET_FILE:MPIAlgorithmsTest>
  )

  set_tests_properties(MPIAlgorithmsTest PROPERTIES LABELS "UnitTest;MPI")
  set(_mpi_tests MPIAlgorithmsTest)

endif()

# MPI tests start their own MPI runtime, so keep other tests from running alongside them
list(APPEND _mpi_tests MPIAlgorithmsTest_BroadcastWorkspaceTest MPIAlgorithmsTest_GatherWorkspacesTest
     MPILifetimeTest_MPILifetimeTest
)
set_tests_properties(${_mpi_tests} PROPERTIES RUN_SERIAL TRUE)
