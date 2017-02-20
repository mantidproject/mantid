def assert_validate_error(caller, error_type, obj):
    try:
        obj.validate()
        raised_correct = False
    except error_type:
        raised_correct = True
    except:  # noqa
        raised_correct = False
    caller.assertTrue(raised_correct)


def assert_raises_nothing(caller, obj):
    obj.validate()
    try:
        obj.validate()
    except:  # noqa
        caller.fail()
