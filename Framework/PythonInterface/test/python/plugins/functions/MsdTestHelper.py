from mantid.simpleapi import FunctionWrapper
from mantid.api import FunctionFactory
import numpy as np

def is_registered(function_name):
    """
    Check whether the function with the specified name has been registered.

    :param function_name: The name of the function to check for registration.
    :return:              A tuple of the status (True if function is registered,
                          false otherwise) and the error message (empty if the
                          function is registered).
    """
    try:
        FunctionFactory.createFunction(function_name)
    except RuntimeError as exc:
        return False, 'Could not create {} function: {}'.format(function, str(exc))
    return True, ""


def check_output(function_name, input, expected_output, tolerance, **function_params):
    func = FunctionWrapper(function_name, **function_params)
    output = func(input)
    return np.allclose(output, expected_output, atol=0.0001), output
