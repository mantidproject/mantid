set(SRC_FILES src/DataProcessorTestHelper.cpp)

set(INC_FILES inc/MantidQtTestHelpers/DataProcessorTestHelper.h)

if(COVERAGE)
  foreach(loop_var ${INC_FILES})
    set_property(GLOBAL APPEND PROPERTY COVERAGE_SRCS "${CMAKE_CURRENT_SOURCE_DIR}/${loop_var}")
  endforeach(loop_var)
endif()

add_library(QtTestHelpers STATIC ${SRC_FILES} ${INC_FILES})
add_library(Mantid::QtTestHelpers ALIAS QtTestHelpers)
set_property(TARGET QtTestHelpers PROPERTY FOLDER "UnitTests")

target_include_directories(QtTestHelpers PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/inc>)

target_link_libraries(
  QtTestHelpers
  PUBLIC Mantid::API
  PRIVATE MantidQtWidgetsCommonQt5 Qt5::Core
)

target_include_directories(QtTestHelpers PRIVATE ${CXXTEST_INCLUDE_DIR})
