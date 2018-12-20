from mantid.simpleapi import (Fit, FunctionWrapper, CreateWorkspace)
from mantid.api import (FunctionFactory, WorkspaceFactory)

from collections import OrderedDict
import numpy as np
import random


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
        return False, 'Could not create {} function: {}'.format(function_name,
                                                                str(exc))
    return True, ""


def create_function_string(function_name, **function_params):
    return str(FunctionWrapper(function_name, **function_params))


def check_output(function_name, input, expected_output, tolerance=0.0001,
                 **function_params):
    r"""Compare values returned by the function evaluation against expected
    values.

    Parameters
    ----------
    function_name : str
        Registered name of the fit function.
    input : list, numpy.ndarray
        Domain values where to evaluate the function.
    expected_output : sequence of floats
        Expected values the function should return.
    tolerance : float
        Absolute tolerance parameter when evaluating expected_output against
        output values
    function_params : dict
        Initial parameter and attribute values.
    Returns
    -------
    list
        [0] Evaluation of the comparison between evaluated and expected values.
        [1] Output of the function call.
    """

    func = FunctionWrapper(function_name, **function_params)
    output = func(input)
    return np.allclose(output, expected_output, atol=tolerance), output


def create_model(function_name, **function_params):
    func = FunctionWrapper(function_name, **function_params)
    return lambda x: func(x)


def create_test_workspace(model, num_bins):
    workspace = WorkspaceFactory.create("Workspace2D", NVectors=1,
                                        XLength=num_bins, YLength=num_bins)

    for i in range(1, num_bins):
        noise = random.uniform(0.8, 1.2)
        x_value = i * 1.2
        workspace.dataX(0)[i] = x_value
        workspace.dataY(0)[i] = noise * model(x_value)
        workspace.dataE(0)[i] = 1
    return workspace


def do_a_fit(x, function, guess, target, fixes=None, atol=0.01):
    r"""Carry out a fit and compare to target parameters

    Parameters
    ----------
    x : sequence of floats
        Domain values for evaluating the function .
    function : str
        Registered function name.
    guess : dict
        Parameter names with their initial values.
    target : dict
        Parameter names with the values to be obtained after the fit.
    fixes : list
        List of fitting parameters to fix during the fit
    atol : float
        Absolute tolerance parameter when evaluating expected_output against
        output values.

    Returns
    -------
    list
        [0] Evaluation of the comparison between evaluated and expected values.
        [1] output of the call to Fit algorithm
    """
    target_model = FunctionWrapper(function, **target)
    y = target_model(x)
    e = np.ones(len(x))
    w = CreateWorkspace(x, y, e, Nspec=1)
    model = FunctionWrapper(function, **guess)
    if fixes is not None:
        [model.fix(p) for p in fixes]
    fit = Fit(model, w, NIterations=2000)
    otarget = OrderedDict(target)
    return np.allclose([fit.Function[p] for p in otarget.keys()],
                       list(otarget.values()), atol), fit
