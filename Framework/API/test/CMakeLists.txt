if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
  add_compile_options(-Wno-uninitialized)
endif()

if(CXXTEST_FOUND)
  include_directories(SYSTEM ${CXXTEST_INCLUDE_DIR})

  cxxtest_add_test(APITest ${TEST_FILES} ${GMOCK_TEST_FILES})

  if(WIN32)
    set(BCRYPT bcrypt)
  endif()

  target_link_libraries(
    APITest
    PRIVATE Mantid::Types
            Mantid::API
            Mantid::DataHandling
            Mantid::Nexus
            Mantid::NexusGeometry
            ${BCRYPT}
            Python::Python
  )

  if(TARGET GTest::gmock AND TARGET GTest::gtest)
    target_link_libraries(APITest PRIVATE GTest::gmock GTest::gtest)
  else()
    target_link_libraries(APITest PRIVATE gmock gtest)
  endif()

  add_framework_test_helpers(APITest)

  add_dependencies(FrameworkTests APITest)
  # Test data
  add_dependencies(APITest UnitTestData)

  # Add to the 'FrameworkTests' group in VS
  set_property(TARGET APITest PROPERTY FOLDER "UnitTests")
endif()
