# Skip these tests entirely on Windows
if(WIN32)
  message(STATUS "Skipping LiveData Python ADARA tests on Windows (no fork support)")
  return()
endif()

set(TEST_PY_FILES
    test_adara_player.py
    test_adara_player_config.py
    test_adara_player_limits.py
    test_adara_player_packet.py
    test_adara_player_play.py
    test_adara_player_record.py
    test_adara_player_server.py
    test_adara_player_summarize.py
    test_adara_player_util.py
)

check_tests_valid(${CMAKE_CURRENT_SOURCE_DIR} ${TEST_PY_FILES})

# Add the packet_playback source module to PYTHONPATH so tests can import player_config, packet_player, etc.
set(_packet_playback_dir ${CMAKE_SOURCE_DIR}/Framework/PythonInterface/mantid/LiveData/ADARA/utils/packet_playback)
set(PYUNITTEST_PYTHONPATH_EXTRA ${_packet_playback_dir})

# Prefix for test=PythonLiveDataADARAdara
pyunittest_add_test(${CMAKE_CURRENT_SOURCE_DIR} python.livedata.adara.packet_playback ${TEST_PY_FILES})

# Append the "adara_player_conf" path to the environment for each of these tests.
foreach(test_file ${TEST_PY_FILES})
  get_filename_component(_suitename ${test_file} NAME_WE)
  set(_test_name "python.livedata.adara.packet_playback.${_suitename}.${_suitename}")
  set_property(
    TEST ${_test_name}
    APPEND
    PROPERTY ENVIRONMENT "adara_player_conf=${_packet_playback_dir}/adara_player_conf.yml"
  )
  set_property(
    TEST ${_test_name}
    APPEND
    PROPERTY ENVIRONMENT "ADARA_PLAYER_LOGS=."
  )
endforeach()

unset(PYUNITTEST_PYTHONPATH_EXTRA)
