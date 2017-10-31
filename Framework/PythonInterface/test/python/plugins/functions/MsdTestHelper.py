from mantid.simpleapi import FunctionWrapper
from mantid.api import FunctionFactory, WorkspaceFactory

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


def create_function_string(function_name, **function_params):
    return str(FunctionWrapper(function_name, **function_params))


def check_output(function_name, input, expected_output, tolerance, **function_params):
    func = FunctionWrapper(function_name, **function_params)
    output = func(input)
    return np.allclose(output, expected_output, atol=0.0001), output


def create_model(function_name, **function_params):
    func = FunctionWrapper(function_name, **function_params)
    return lambda x: func(x)


def create_test_workspace(model, num_bins):
    import random
    workspace = WorkspaceFactory.create("Workspace2D", NVectors=1, XLength=num_bins, YLength=num_bins)

    for i in range(1, num_bins):
        noise = random.uniform(0.8, 1.2)
        x_value = i * 1.2
        workspace.dataX(0)[i] = x_value
        workspace.dataY(0)[i] = noise * model(x_value)
        workspace.dataE(0)[i] = 1
    return workspace
