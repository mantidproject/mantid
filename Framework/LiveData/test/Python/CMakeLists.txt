set(TEST_PY_FILES
    test_adara_player.py
    test_adara_player_play.py
    test_adara_player_record.py
    test_adara_player_packet.py
    test_adara_player_util.py
    test_adara_player_limits.py
    test_adara_player_server.py
    test_adara_player_summarize.py
)

# Prefix for test name=python.utils.livedata
set(PYUNITTEST_PYTHONPATH_EXTRA ${LIVEDATA_SOURCE_DIR}/ADARA/utils/packet_playback)
pyunittest_add_test(${CMAKE_CURRENT_SOURCE_DIR} python.utils.livedata ${TEST_PY_FILES})

# Append the "adara_player_conf" path to the environment for each of these tests.
foreach(test_file ${TEST_PY_FILES})
  get_filename_component(_suitename ${test_file} NAME_WE)
  set(_test_name "python.utils.livedata.${_suitename}.${_suitename}")
  set_property(
    TEST ${_test_name}
    APPEND
    PROPERTY ENVIRONMENT "adara_player_conf=${LIVEDATA_SOURCE_DIR}/ADARA/utils/packet_playback/adara_player_conf.yml"
  )
  set_property(
    TEST ${_test_name}
    APPEND
    PROPERTY ENVIRONMENT "LOGS='.'"
  )
endforeach()
