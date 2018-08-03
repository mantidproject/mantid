from __future__ import (absolute_import, division, print_function)
from collections import namedtuple
from mantid.api import MatrixWorkspace
from mantid.dataobjects import EventWorkspace
from sans.common.general_functions import create_unmanaged_algorithm
from sans.common.constants import EMPTY_NAME
from sans.common.enums import SaveType

ZERO_ERROR_DEFAULT = 1e6

file_format_with_append = namedtuple('file_format_with_append', 'file_format, append_file_format_name')


def save_to_file(workspace, file_format, file_name):
    """
    Save a workspace to a file.

    :param workspace: the workspace to save.
    :param file_format: the selected file format type.
    :param file_name: the file name.
    :return:
    """
    save_options = {"InputWorkspace": workspace}
    save_alg = get_save_strategy(file_format, file_name, save_options)
    save_alg.setRethrows(True)
    save_alg.execute()


def get_save_strategy(file_format_bundle, file_name, save_options):
    """
    Provide a save strategy based on the selected file format

    :param file_format_bundle: the selected file_format_bundle
    :param file_name: the name of the file
    :param save_options: the save options such as file name and input workspace
    :return: a handle to a save algorithm
    """
    file_format = file_format_bundle.file_format
    if file_format is SaveType.Nexus:
        file_name = get_file_name(file_format_bundle, file_name, "", ".nxs")
        save_name = "SaveNexusProcessed"
    elif file_format is SaveType.CanSAS:
        file_name = get_file_name(file_format_bundle, file_name, "", ".xml")
        save_name = "SaveCanSAS1D"
    elif file_format is SaveType.NXcanSAS:
        file_name = get_file_name(file_format_bundle, file_name, "_nxcansas", ".h5")
        save_name = "SaveNXcanSAS"
    elif file_format is SaveType.NistQxy:
        file_name = get_file_name(file_format_bundle, file_name, "_nistqxy", ".dat")
        save_name = "SaveNISTDAT"
    elif file_format is SaveType.RKH:
        file_name = get_file_name(file_format_bundle, file_name, "", ".txt")
        save_name = "SaveRKH"
    elif file_format is SaveType.CSV:
        file_name = get_file_name(file_format_bundle, file_name, "", ".csv")
        save_name = "SaveCSV"
    else:
        raise RuntimeError("SaveWorkspace: The requested data {0} format is "
                           "currently not supported.".format(file_format))
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
    clone_options = {"InputWorkspace": workspace,
                     "OutputWorkspace": EMPTY_NAME}
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
    if not isinstance(workspace, MatrixWorkspace) or isinstance(workspace,EventWorkspace):
        raise ValueError('Cannot remove zero errors from a workspace which is not a MatrixWorkspace.')

    # Iterate over the workspace and replace the zero values with a large default value
    number_of_spectra = workspace.getNumberHistograms()
    errors = workspace.dataE
    for index in range(0, number_of_spectra):
        spectrum = errors(index)
        spectrum[spectrum <= 0.0] = ZERO_ERROR_DEFAULT
