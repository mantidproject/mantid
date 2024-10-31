# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
"""Defines a set of helpers wrapped around the C++ TestHelpers package that
are for use in unit tests only!
"""

from contextlib import contextmanager

# Import mantid to set MANTIDPATH for any ConfigService call that may be done
import mantid

# Define some pure-Python functions to add to the mix


def run_algorithm(name, **kwargs):
    """Run a named algorithm and return the
    algorithm handle

    Parameters:
        name - The name of the algorithm
        kwargs - A dictionary of property name:value pairs
    """
    alg = create_algorithm(name, **kwargs)
    alg.execute()
    return alg


def create_algorithm(name, **kwargs):
    """Create a named algorithm, set the properties given by the keywords and return the
    algorithm handle WITHOUT executing the algorithm

    Useful keywords:
      - child: Makes algorithm a child algorithm
      - rethrow: Causes exceptions to be rethrown on execution

    Parameters:
        name - The name of the algorithm
        kwargs - A dictionary of property name:value pairs
    @returns The algorithm handle
    """
    # Initialize the whole framework
    import mantid.simpleapi

    if "Version" in kwargs:
        alg = mantid.api.AlgorithmManager.createUnmanaged(name, kwargs["Version"])
        del kwargs["Version"]
    else:
        alg = mantid.api.AlgorithmManager.createUnmanaged(name)
    alg.initialize()
    # Avoid problem that Load needs to set Filename first if it exists
    if name == "Load" and "Filename" in kwargs:
        alg.setPropertyValue("Filename", kwargs["Filename"])
        del kwargs["Filename"]
    if "child" in kwargs:
        alg.setChild(True)
        del kwargs["child"]
        if "OutputWorkspace" in alg:
            alg.setPropertyValue("OutputWorkspace", "UNUSED_NAME_FOR_CHILD")
    if "rethrow" in kwargs:
        alg.setRethrows(True)
        del kwargs["rethrow"]
    alg.setProperties(kwargs)
    return alg


# Case difference is to be consistent with the unittest module
def assertRaisesNothing(testobj, callable, *args, **kwargs):
    """
    unittest does not have an assertRaisesNothing. This
    provides that functionality

    Parameters:
        testobj  - A unittest object
        callable - A callable object
        *args    - Positional arguments passed to the callable as they are
        **kwargs - Keyword arguments, passed on as they are
    """
    try:
        return callable(*args, **kwargs)
    except Exception as exc:
        testobj.fail("Assertion error. An exception was caught where none was expected in %s. Message: %s" % (callable.__name__, str(exc)))


def assert_called_with_partial(_mock_self, *args, **kwargs):
    """This is similar to assert_called_with but passes if the function is called with provided arguments and keywords,
    which can be a subset of the full list of arguments passed.
    Credit: https://stackoverflow.com/questions/52647476/python-unit-test-assert-called-with-partial
    """
    self = _mock_self
    if self.call_args is None:
        expected = self._format_mock_call_signature(args, kwargs)
        raise AssertionError("Expected call: %s\nNot called" % (expected,))

    def _error_message():
        msg = self._format_mock_failure_message(args, kwargs)
        return msg

    expected = self._call_matcher((args, kwargs))
    expected_args, expected_kwargs = expected
    actual_args, actual_kwargs = self._call_matcher(self.call_args)
    if actual_args[: len(expected_args)] != expected_args or not (expected_kwargs.items() <= actual_kwargs.items()):
        cause = expected if isinstance(expected, Exception) else None
        raise AssertionError(_error_message()) from cause


def assert_any_call_partial(_mock_self, *args, **kwargs):
    """This is similar to assert_any_call but passes if the function is called with provided arguments and keywords,
    which can be a subset of the full list of arguments passed.
    Adapted from: https://stackoverflow.com/questions/52647476/python-unit-test-assert-called-with-partial and the
    unittest assert_any_call function
    """
    self = _mock_self
    if self.call_args is None:
        expected = self._format_mock_call_signature(args, kwargs)
        raise AssertionError("Expected call: %s\nNot called" % (expected,))

    def _error_message():
        msg = self._format_mock_failure_message(args, kwargs)
        return msg

    expected = self._call_matcher((args, kwargs))
    actual = [self._call_matcher(c) for c in self.call_args_list]
    expected_args, expected_kwargs = expected
    for actual_args, actual_kwargs in actual:
        if actual_args[: len(expected_args)] == expected_args and (expected_kwargs.items() <= actual_kwargs.items()):
            return
    cause = expected if isinstance(expected, Exception) else None
    raise AssertionError(_error_message()) from cause


def can_be_instantiated(cls):
    """The Python unittest assertRaises does not
    seem to catch the assertion raised by being unable
    to instantiate a class (or maybe it's just the boost
    python stuff).
    In any case this little function tests for it and returns
    a boolean
    """
    try:
        cls()
        result = True
    except RuntimeError:
        result = False
    return result


@contextmanager
def temporary_config():
    """
    Creates a backup of the current system configuration and restores
    is when the context is existed.
    """
    try:
        config = mantid.kernel.ConfigService.Instance()
        backup = {key: config[key] for key in config.keys()}
        yield
    finally:
        for key, val in backup.items():
            config[key] = val
