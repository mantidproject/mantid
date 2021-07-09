# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from Muon.GUI.Common.ADSHandler.ADS_calls import check_if_workspace_exist, retrieve_ws

X_OFFSET = 0.001


def x_limits_of_workspace(workspace_name: str, default_limits: tuple) -> tuple:
    """Returns the x data limits of a provided workspace."""
    if workspace_name is not None and check_if_workspace_exist(workspace_name):
        x_data = retrieve_ws(workspace_name).dataX(0)
        if len(x_data) > 0:
            x_data.sort()
            x_lower, x_higher = x_data[0], x_data[-1]
            if x_lower == x_higher:
                return x_lower - X_OFFSET, x_higher + X_OFFSET
            return x_lower, x_higher
    return default_limits
