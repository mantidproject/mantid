# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2023 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +

from mantid.simpleapi import CompareWorkspaces


def assert_almost_equal(Workspace1, Workspace2, rtol=0.0, atol=0.0):
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
        If atol and rtol are both provided

    """

    if rtol != 0.0 and atol != 0.0:
        raise ValueError("Specify rtol or atol, not both")
    _rel = False
    tolerance = 1e-10
    if atol:
        tolerance = atol

    if rtol:
        tolerance = rtol
        _rel = True

    result, message = CompareWorkspaces(Workspace1, Workspace2, Tolerance=tolerance, ToleranceRelErr=_rel)
    msg_dict = message.toDict()
    if not result:
        msg = ", ".join(msg_dict["Message"]) + f", Workspaces are not within tolerance ({tolerance})"
        raise AssertionError(msg)
