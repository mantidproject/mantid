def assert_called(func, n = 1):
    # Calls to this method with n = 1 can
    # be replaced with a call to mock.assert_called
    # when python 2 support is dropped.
    assert(func.call_count >= n)
