if(CXXTEST_FOUND)
  include_directories(SYSTEM ${CXXTEST_INCLUDE_DIR})

  # The actual test suite
  cxxtest_add_test(CatalogTest ${TEST_FILES})
  target_link_libraries(CatalogTest PRIVATE Mantid::Catalog gmock)
  add_dependencies(FrameworkTests CatalogTest)
  add_framework_test_helpers(CatalogTest)

  # Add to the 'FrameworkTests' group in VS
  set_property(TARGET CatalogTest PROPERTY FOLDER "UnitTests")
endif()
