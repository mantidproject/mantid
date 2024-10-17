# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2023 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from collections import namedtuple

from mantid.simpleapi import CompareWorkspaces
from mantid.kernel import Property

# the absolute tolerance to be used if neither atol nor rtol specified
DEFAULT_ABS_TOLERANCE = 1.0e-10

CompareResult = namedtuple("CompareResult", ["rel_res", "rel_msg", "abs_res", "abs_msg"])


def _compare_workspaces_result(Workspace1, Workspace2, rtol=Property.EMPTY_DBL, atol=Property.EMPTY_DBL, **kwargs):
    """
    Returns the result of CompareWorkspaces for the given rtol and/or atol

    If neither rtol nor atol is specified, the comparison is done using the default absolute
    tolerance

    Parameters
    ----------
    Workspace1, Workspace2 : Workspace
        Input workspaces to compare.
    rtol : float
        The relative tolerance parameter.
    atol : float
        The absolute tolerance parameter.
    kwargs: dict
        Other options available to CompareWorkspaces.

    Returns
    -------
    A namedtuple `CompareResult` containing:
     - `rel_res` - if relative tolerance is used, True or False, otherwise None
     - `rel_msg` - if relative tolerance is used, message with mismatches, otherwise None
     - `abs_res` - if absolute tolerance is used, True or False, otherwise None
     - `abs_msg` - if absolute tolerance is used, message with mismatches, otherwise None

    Raises
    -------
    ValueError
        If atol or rtol are below zero
    ValueError
        If kwargs contains keys that should instead be set by atol or rtol

    """
    # check arguments
    if len(set(kwargs.keys()).intersection({"Tolerance", "ToleranceRelErr"})):
        raise ValueError("Tolerance, ToleranceRelErr cannot be passed as additional parameters: set atol or rtol instead")
    if atol < 0.0:
        raise ValueError("Absolute tolerance must be nonnegative")
    if rtol < 0.0:
        raise ValueError("Relative tolerance must be nonnegative")

    use_relative = rtol != Property.EMPTY_DBL
    use_absolute = atol != Property.EMPTY_DBL

    # perform the required comparisons

    # if neither set, do simple absolute comparison
    if not use_absolute and not use_relative:
        use_absolute = True
        atol = DEFAULT_ABS_TOLERANCE

    # if rtol set, perform relative comparison
    rel_res, rel_msg = None, None
    if use_relative:
        rel_res, message = CompareWorkspaces(Workspace1, Workspace2, Tolerance=rtol, ToleranceRelErr=True, **kwargs)
        rel_msg = message.toDict()

    # if atol set, perform absolute comparison
    abs_res, abs_msg = None, None
    if use_absolute:
        abs_res, message = CompareWorkspaces(Workspace1, Workspace2, Tolerance=atol, ToleranceRelErr=False, **kwargs)
        abs_msg = message.toDict()

    return CompareResult(rel_res=rel_res, rel_msg=rel_msg, abs_res=abs_res, abs_msg=abs_msg)


def assert_almost_equal(Workspace1, Workspace2, rtol=Property.EMPTY_DBL, atol=Property.EMPTY_DBL, **kwargs):
    """
    Raises an assertion error if two workspaces are not within specified tolerance.

    Parameters
    ----------
    Workspace1, Workspace2 : Workspace
        Input workspaces to compare.
    rtol : float
        The relative tolerance parameter.
    atol : float
        The absolute tolerance parameter.
    kwargs: dict
        Other options available to CompareWorkspaces.

    Raises
    -------
    AssertionError
        If Workspace1 and Workspace2 are not equal up to specified precision
    ValueError
        If atol or rtol are below zero
    ValueError
        If kwargs contains keys that should instead be set by atol or rtol

    """
    res = _compare_workspaces_result(Workspace1, Workspace2, rtol, atol, **kwargs)

    # validate the comparison results
    msg = ""
    if res.rel_res is False:
        msg += ", ".join(res.rel_msg["Message"]) + f", Workspaces are not within relative tolerance ({rtol})"
    if res.abs_res is False:
        msg += ", ".join(res.abs_msg["Message"]) + f", Workspaces are not within absolute tolerance ({atol})"
    if res.rel_res is False or res.abs_res is False:
        raise AssertionError(msg)


def assert_not_equal(Workspace1, Workspace2, rtol=Property.EMPTY_DBL, atol=Property.EMPTY_DBL, **kwargs):
    """
    Raises an assertion error if two workspaces are equal within specified tolerance.

    Parameters
    ----------
    Workspace1, Workspace2 : Workspace
        Input workspaces to compare.
    rtol : float
        The relative tolerance parameter.
    atol : float
        The absolute tolerance parameter.
    kwargs: dict
        Other options available to CompareWorkspaces.

    Raises
    -------
    AssertionError
        If Workspace1 and Workspace2 are equal up to specified precision
    ValueError
        If atol or rtol are below zero or both are specified.
    ValueError
        If kwargs contains keys that should instead be set by atol or rtol

    """
    use_relative = rtol != Property.EMPTY_DBL
    use_absolute = atol != Property.EMPTY_DBL
    if use_relative and use_absolute:
        raise ValueError("Cannot specify both relative tolerance and absolute tolerance")

    res = _compare_workspaces_result(Workspace1, Workspace2, rtol, atol, **kwargs)

    # validate the comparison results
    msg = ""
    if res.rel_res:
        msg += f"Workspaces are equal within relative tolerance ({rtol})"
    if res.abs_res:
        msg += f"Workspaces are equal within absolute tolerance ({atol})"
    if res.rel_res or res.abs_res:
        raise AssertionError(msg)
