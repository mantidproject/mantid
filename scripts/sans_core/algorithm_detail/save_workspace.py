# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from collections import namedtuple
from mantid.api import MatrixWorkspace
from mantid.dataobjects import EventWorkspace
from sans_core.common.general_functions import create_unmanaged_algorithm
from sans_core.common.constants import EMPTY_NAME
from sans_core.common.enums import SaveType

# from sans_core.algorithm_detail.strip_end_nans_and_infs import strip_end_nans

ZERO_ERROR_DEFAULT = 1e6

file_format_with_append = namedtuple("file_format_with_append", "file_format, append_file_format_name")


def save_to_file(workspace, file_format, file_name, additional_properties, additional_run_numbers):
    """
    Save a workspace to a file.

    :param workspace: the workspace to save.
    :param file_format: the selected file format type.
    :param file_name: the file name.
    :param additional_properties: a dict of additional save algorithm inputs
            e.g. Transmission and TransmissionCan for SaveCanSAS1D-v2
    :param additional_run_numbers: a dict of workspace type to run number. Used in SaveNXCanSAS only.
    :return:
    """
    save_options = {"InputWorkspace": workspace}
    save_alg = get_save_strategy(file_format, file_name, save_options, additional_properties, additional_run_numbers)
    save_alg.setRethrows(True)
    save_alg.execute()


def get_save_strategy(file_format_bundle, file_name, save_options, additional_properties, additional_run_numbers):
    """
    Provide a save strategy based on the selected file format

    :param file_format_bundle: the selected file_format_bundle
    :param file_name: the name of the file
    :param save_options: the save options such as file name and input workspace
    :param additional_properties: a dict of additional inputs for SaveCanSAS algorithm
    :param additional_run_numbers: a dict of workspace type to run number
    :return: a handle to a save algorithm
    """
    file_format = file_format_bundle.file_format
    if file_format is SaveType.NEXUS:
        file_name = get_file_name(file_format_bundle, file_name, "", ".nxs")
        save_name = "SaveNexusProcessed"
    elif file_format is SaveType.CAN_SAS:
        file_name = get_file_name(file_format_bundle, file_name, "", ".xml")
        save_name = "SaveCanSAS1D"
        save_options.update(additional_properties)
        save_options.update(additional_run_numbers)
    elif file_format is SaveType.NX_CAN_SAS:
        file_name = get_file_name(file_format_bundle, file_name, "_nxcansas", ".h5")
        save_name = "SaveNXcanSAS"
        save_options.update(additional_properties)
        save_options.update(additional_run_numbers)
    elif file_format is SaveType.NIST_QXY:
        file_name = get_file_name(file_format_bundle, file_name, "_nistqxy", ".dat")
        save_name = "SaveNISTDAT"
    elif file_format is SaveType.RKH:
        file_name = get_file_name(file_format_bundle, file_name, "", ".txt")
        save_name = "SaveRKH"
        save_options.update({"Append": False})
    elif file_format is SaveType.CSV:
        file_name = get_file_name(file_format_bundle, file_name, "", ".csv")
        save_name = "SaveCSV"
    else:
        raise RuntimeError("SaveWorkspace: The requested data {0} format is " "currently not supported.".format(file_format))
    save_options.update({"Filename": file_name})
    return create_unmanaged_algorithm(save_name, **save_options)


def get_file_name(file_format, file_name, post_fix, extension):
    if file_format.append_file_format_name:
        file_name += post_fix
    file_name += extension
    return file_name


def get_zero_error_free_workspace(workspace):
    """
    Creates a cloned workspace where all zero-error values have been replaced with a large value

    :param workspace: The input workspace
    :return: The zero-error free workspace
    """
    clone_name = "CloneWorkspace"
    clone_options = {"InputWorkspace": workspace, "OutputWorkspace": EMPTY_NAME}
    clone_alg = create_unmanaged_algorithm(clone_name, **clone_options)
    clone_alg.execute()
    cloned_workspace = clone_alg.getProperty("OutputWorkspace").value
    remove_zero_errors_from_workspace(cloned_workspace)
    return cloned_workspace


def remove_zero_errors_from_workspace(workspace):
    """
    Removes the zero errors from a matrix workspace

    :param workspace: The workspace which will have its zero error values removed.
    :return: A zero-error free workspace
    """
    # Make sure we are dealing with a MatrixWorkspace
    if not isinstance(workspace, MatrixWorkspace) or isinstance(workspace, EventWorkspace):
        raise ValueError("Cannot remove zero errors from a workspace which is not a MatrixWorkspace.")

    # Uncomment the next line and tests fail for checking error values should not be zero, and
    # comparing loaded workspace to calculated workspace. If we want to remove RuntimeWarning for nan
    # values strip_end_nans should be moved up the workflow
    # workspace = strip_end_nans(workspace, None)

    # Iterate over the workspace and replace the zero values with a large default value
    number_of_spectra = workspace.getNumberHistograms()
    errors = workspace.dataE
    for index in range(0, number_of_spectra):
        spectrum = errors(index)
        spectrum[spectrum <= 0.0] = ZERO_ERROR_DEFAULT
