if(CXXTEST_FOUND)
  include_directories(SYSTEM ${CXXTEST_INCLUDE_DIR})

  # The actual test suite
  cxxtest_add_test(CatalogTest ${TEST_FILES})
  target_link_libraries(CatalogTest PRIVATE Mantid::Catalog)
  add_dependencies(FrameworkTests CatalogTest)
  add_framework_test_helpers(CatalogTest)

  if(TARGET GTest::gmock_main)
    target_link_libraries(CatalogTest PRIVATE GTest::gmock_main)
  elseif(TARGET GTest::gmock AND TARGET GTest::gtest)
    target_link_libraries(CatalogTest PRIVATE GTest::gmock GTest::gtest)
  else()
    target_link_libraries(CatalogTest PRIVATE gmock gtest)
  endif()

  # Add to the 'FrameworkTests' group in VS
  set_property(TARGET CatalogTest PROPERTY FOLDER "UnitTests")
endif()
