pyunittest_add_test(
  ${CMAKE_CURRENT_SOURCE_DIR}
  python.SANS
  SANSBatchModeTest.py
  SANSCentreFinderTest.py
  SANSCommandInterfaceTest.py
  SansIsisGuiSettings.py
  SANSReducerTest.py
  SANSReductionStepsUserFileTest.py
)

add_subdirectory(algorithm_detail)
add_subdirectory(common)
add_subdirectory(state)
add_subdirectory(user_file)
