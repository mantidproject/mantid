# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
from __future__ import (absolute_import, division, print_function)


def _isDOS(workspace):
    """Return True if workspace looks like a valid DOS workspace, False otherwise."""
    singleHistogram = workspace.getNumberHistograms() == 1
    unitsInDeltaE = workspace.getAxis(0).getUnit().unitID() == 'DeltaE'
    return singleHistogram and unitsInDeltaE


def _isSofQW(workspace):
    """Return True if workspace looks like a valid S(Q,E) workspace, False otherwise."""
    validUnits = ['q', 'Energy transfer']
    unit1 = workspace.getAxis(0).getUnit().name()
    unit2 = workspace.getAxis(1).getUnit().name()
    return unit1 != unit2 and unit1 in validUnits and unit2 in validUnits


def _singlehistogramordie(workspace):
    """Raise an exception if workspace has more than one histogram."""
    if workspace.getNumberHistograms() > 1:
        raise RuntimeError("The workspace '{}' has more than one histogram.".format(workspace))


def _styleordie(style):
    """Raise an exception if style is not a valid style string."""
    if not isinstance(style, str):
        raise RuntimeError("The 'style' argument '{}' is not a valid string.".format(str(style)))
    if 'm' not in style and 'l' not in style:
        raise RuntimeError("The 'style' argument '{}' does not contain either 'm' or 'l'.".format(style))
