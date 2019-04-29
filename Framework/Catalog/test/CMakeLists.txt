if(CXXTEST_FOUND)
  include_directories(SYSTEM ${CXXTEST_INCLUDE_DIR})

  include_directories(../../TestHelpers/inc)
  set(TESTHELPER_SRCS ../../TestHelpers/src/ONCatHelper.cpp)

  # The actual test suite
  cxxtest_add_test(CatalogTest ${TEST_FILES})
  target_link_libraries(CatalogTest
                        LINK_PRIVATE
                        ${TCMALLOC_LIBRARIES_LINKTIME}
                        ${MANTIDLIBS}
                        Catalog)
  add_dependencies(FrameworkTests CatalogTest)

  # Add to the 'FrameworkTests' group in VS
  set_property(TARGET CatalogTest PROPERTY FOLDER "UnitTests")
endif()
