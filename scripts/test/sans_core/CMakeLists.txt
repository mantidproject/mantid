pyunittest_add_test(
  ${CMAKE_CURRENT_SOURCE_DIR}
  python.sans_core
  SANSBatchModeTest.py
  SANSCentreFinderTest.py
  SANSCommandInterfaceTest.py
  SansIsisGuiSettings.py
  SANSReducerTest.py
  SANSReductionStepsUserFileTest.py
)

add_subdirectory(algorithm_detail)
add_subdirectory(common)
add_subdirectory(command_interface)
add_subdirectory(state)
add_subdirectory(user_file)
