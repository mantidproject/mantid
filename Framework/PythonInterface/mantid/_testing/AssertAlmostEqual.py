# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2023 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +

from mantid.simpleapi import CompareWorkspaces
from mantid.kernel import Property


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
        If atol and rtol are both not provided

    """
    # check arguments
    if len(set(kwargs.keys()).intersection({"Workspace1", "Workspace2", "Tolerance", "ToleranceRelErr"})):
        raise ValueError("Workspace1, Workspace2, Tolerance, ToleranceRelErr cannot be passed as additional parameters")

    if rtol == Property.EMPTY_DBL and atol == Property.EMPTY_DBL:
        raise ValueError("Specify rtol or atol")

    if rtol > Property.EMPTY_DBL:
        result, message = CompareWorkspaces(Workspace1, Workspace2, Tolerance=rtol, ToleranceRelErr=True, **kwargs)
        msg_dict = message.toDict()
        if not result:
            msg = ", ".join(msg_dict["Message"]) + f", Workspaces are not within relative tolerance ({rtol})"
            raise AssertionError(msg)

    if atol > Property.EMPTY_DBL:
        result, message = CompareWorkspaces(Workspace1, Workspace2, Tolerance=atol, ToleranceRelErr=False, **kwargs)
        msg_dict = message.toDict()
        if not result:
            msg = ", ".join(msg_dict["Message"]) + f", Workspaces are not within absolute tolerance ({atol})"
            raise AssertionError(msg)
