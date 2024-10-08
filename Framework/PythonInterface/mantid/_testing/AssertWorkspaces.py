# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2023 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +

from mantid.simpleapi import CompareWorkspaces
from mantid.kernel import Property

# the absolute tolerance to be used if neither atol nor rtol specified
DEFAULT_ABS_TOLERANCE = 1.0e-10


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

    Raises
    -------
    AssertionError
        If Workspace1 and Workspace2 are not equal up to specified precision
    ValueError
        If atol or rtol are below zero
    ValueError
        If kwargs contains keys that cshould instead be set by atol or rtol

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
        res, message = CompareWorkspaces(Workspace1, Workspace2, Tolerance=DEFAULT_ABS_TOLERANCE, ToleranceRelErr=False, **kwargs)
        msg_dict = message.toDict()
        if res:
            return
        else:
            msg = ", ".join(msg_dict["Message"]) + f", Workspaces are not within default tolerance ({DEFAULT_ABS_TOLERANCE})"
            raise AssertionError(msg)

    # if rtol set, perform relative comparison
    rel_res, rel_msg = True, {}
    if use_relative:
        rel_res, message = CompareWorkspaces(Workspace1, Workspace2, Tolerance=rtol, ToleranceRelErr=True, **kwargs)
        rel_msg = message.toDict()

    # if atol set, perform absolute comparison
    abs_res, abs_msg = True, {}
    if use_absolute:
        abs_res, message = CompareWorkspaces(Workspace1, Workspace2, Tolerance=atol, ToleranceRelErr=False, **kwargs)
        abs_msg = message.toDict()

    # validate the comparison results

    msg = ""
    if not rel_res:
        msg += ", ".join(rel_msg["Message"]) + f", Workspaces are not within relative tolerance ({rtol})"
    if not abs_res:
        msg += ", ".join(abs_msg["Message"]) + f", Workspaces are not within absolute tolerance ({atol})"
    if not rel_res or not abs_res:
        raise AssertionError(msg)
