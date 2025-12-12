set(SRC_FILES src/Json.cpp)

set(INC_FILES inc/MantidJson/Json.h)

set(TEST_FILES test/JsonTest.h)

add_library(Json ${SRC_FILES} ${INC_FILES})
add_library(Mantid::Json ALIAS Json)

target_link_libraries(Json PUBLIC JsonCPP::jsoncpp)
set_target_properties(Json PROPERTIES OUTPUT_NAME MantidJson)

if(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
  set_target_properties(Json PROPERTIES INSTALL_RPATH "@loader_path/../Contents/MacOS")
elseif(${CMAKE_SYSTEM_NAME} STREQUAL "Linux")
  set_target_properties(Json PROPERTIES INSTALL_RPATH "\$ORIGIN/../${LIB_DIR}")
endif()

target_include_directories(
  Json PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/inc> $<INSTALL_INTERFACE:include/Mantid>
)

# Add to the 'Framework' group in VS
set_property(TARGET Json PROPERTY FOLDER "MantidFramework")

# Auto-generate exports header
target_include_directories(
  Json PUBLIC $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/Framework/Json> $<INSTALL_INTERFACE:include/Mantid>
)

generate_mantid_export_header(Json FALSE)

if(CXXTEST_FOUND)

  cxxtest_add_test(JsonTest ${TEST_FILES})
  target_include_directories(JsonTest SYSTEM PRIVATE ${CXXTEST_INCLUDE_DIR} ${JSONCPP_INCLUDE_DIR})
  target_link_libraries(JsonTest PUBLIC ${JSONCPP_LIBRARIES} Json)

  add_dependencies(FrameworkTests JsonTest)

  if(TARGET GTest::gmock AND TARGET GTest::gtest)
    target_link_libraries(JsonTest PRIVATE GTest::gmock GTest::gtest)
  else()
    target_link_libraries(JsonTest PRIVATE gmock gtest)
  endif()

  # Add to the 'FrameworkTests' group in VS
  set_property(TARGET JsonTest PROPERTY FOLDER "UnitTests")

endif()

# Installation settings
set(TARGET_EXPORT_NAME "MantidJsonTargets")
mtd_install_framework_lib(TARGETS Json INSTALL_EXPORT_FILE EXPORT_NAME ${TARGET_EXPORT_NAME})
