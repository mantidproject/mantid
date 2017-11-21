def assert_called(func):
    # This method can be replaced with a call to mock.assert_called
    # when python 2 support is dropped.
    assert(func.call_count >= 1)
