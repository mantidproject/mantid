set (TEST_PY_FILES
    test_adara_player.py
    test_adara_player_play.py
    test_adara_player_record.py
    test_adara_player_packet.py
    test_adara_player_util.py
)

# python unit tests
if(PYUNITTEST_FOUND)
  pyunittest_add_test(${CMAKE_CURRENT_SOURCE_DIR} python.utils.livedata ${TEST_PY_FILES})
endif()
