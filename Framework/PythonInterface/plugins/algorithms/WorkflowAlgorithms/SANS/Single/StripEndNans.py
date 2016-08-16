from math import (isinf, isnan)
from SANS2.Common.SANSConstants import SANSConstants
from SANS2.Common.SANSFunctions import create_unmanaged_algorithm


def strip_end_nans(workspace):
    """
    This function removes the INFs and NANs from the start and end of a 1D workspace.

    :param workspace: The workspace which is about to be trimmed
    :return: A trimmed NAN- and INF-trimmed workspace
    """
    # If the workspace is larger than 1D, then there is nothing we can do
    if workspace.getNumberHistograms() > 1:
        return workspace

    data = workspace.readY(0)
    # Find the index at which the first legal value appears
    start_index = next((index for index in range(0, len(data)) if is_valid_data(data[index])), None)
    end_index = next((index for index in reversed(range(0, len(data))) if is_valid_data(data[index])), None)

    # If an index was not found then we return the current workspace. This means that all entries are either INFs
    # or NANs.
    if start_index is None or end_index is None:
        return workspace

    # Get the corresponding Q values
    q_values = workspace.readX(0)

    start_q = q_values[start_index]

    # Make sure we're inside the bin that we want to crop. This is part of the old framework. It looks like a bug fix,
    # hence we leave it in here for now. In general this is risky, and it should be a fraction of a bin width by which
    # we increase the end value
    is_point_data = len(workspace.dataX(0)) == len(workspace.dataY(0))
    if is_point_data:
        end_q = 1.001 * q_values[end_index]
    else:
        end_q = 1.001 * q_values[end_index + 1]

    # Crop the workspace in place
    crop_name = "CropWorkspace"
    crop_options = {SANSConstants.input_workspace: workspace,
                    "XMin": start_q,
                    "XMax": end_q}
    crop_alg = create_unmanaged_algorithm(crop_name, **crop_options)
    crop_alg.setProperty(SANSConstants.output_workspace, "dummy")
    crop_alg.execute()
    ws = crop_alg.getProperty(SANSConstants.output_workspace).value
    return ws


def is_valid_data(value):
    return not isinf(value) and not isnan(value)
