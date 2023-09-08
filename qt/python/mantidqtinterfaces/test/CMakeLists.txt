set(TEST_PY_FILES PyChopInstrumentTest.py)

if(ENABLE_WORKBENCH)
  set(PYUNITTEST_QT_API pyqt5) # force to use qt5
  pyunittest_add_test(${CMAKE_CURRENT_SOURCE_DIR} python.scripts ${TEST_PY_FILES})
  unset(PYUNITTEST_QT_API)
endif()

# Add test directories
add_subdirectory(Muon)
add_subdirectory(MultiPlotting)
add_subdirectory(sample_transmission_calculator)
add_subdirectory(SimpleScanViewer)
add_subdirectory(TOFTOF)
add_subdirectory(dns_powder_tof)
add_subdirectory(dns_powder_elastic)
