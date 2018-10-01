from __future__ import (absolute_import, division, print_function)

import os

allowed_instruments = ["EMU", "MUSR", "CHRONUS", "HIFI"]
allowed_extensions = ["nxs"]


def filter_for_extensions(extensions):
    """Filter for file browser"""
    str_list = ["*." + str(ext) for ext in extensions]
    return "Files (" + ", ".join(str_list) + ")"


def get_instrument_directory(instrument):
    """
    If instrument is supported, returns the directory name on the ISIS network
    in which its data can be found
    """
    if instrument in allowed_instruments:
        instrument_directory = instrument
        if instrument == "CHRONUS":
            instrument_directory = "NDW1030"
        return instrument_directory
    else:
        return None


def check_file_exists(filename):
    return os.path.isfile(filename)


def get_current_run_filename(instrument):
    """
    If instrument is supported, attempts to find the file on the ISIS network which
    contains the data from its current (most up-to-date) run.
    """

    instrument_directory = get_instrument_directory(instrument)
    if instrument_directory is None:
        return ""

    autosave_file_name = _instrument_data_directory(instrument) + os.sep + "autosave.run"
    autosave_points_to = ""
    if not check_file_exists(autosave_file_name):
        raise ValueError("Cannot find file : " + autosave_file_name)
    with open(autosave_file_name, 'r') as autosave_file:
        for line in autosave_file:
            if os.path.isfile(line):
                autosave_points_to = line
    if autosave_points_to == "":
        # Default to auto_A (replicates MuonAnalysis 1.0 behaviour)
        current_run_filename = os.sep + os.sep + instrument_directory + os.sep + "data" \
                               + os.sep + instrument_directory + "auto_A.tmp"
    else:
        current_run_filename = os.sep + os.sep + instrument_directory + os.sep + "data" \
                               + os.sep + autosave_points_to
    return current_run_filename


def format_run_for_file(run):
    return "{0:08d}".format(run)


def _instrument_data_directory(instrument):
    """The directory which stores the data for a particular instrument"""
    return os.sep + os.sep + get_instrument_directory(instrument) + os.sep + "data"


def file_path_for_instrument_and_run(instrument, run):
    """Returns the path to the data file for a given instrument/run"""
    base_dir = _instrument_data_directory(instrument)
    file_name = instrument + format_run_for_file(run) + ".nxs"
    return base_dir.lower() + os.sep + file_name


def remove_duplicated_files_from_list(file_list):
    """
    Split filenames from their paths, and remove duplicates, keeping the
    first ocurrence in the list.

    Example

    input :
        ["C:\dir1\file1.nxs","C:\dir1\file2.nxs","C:\dir1\dir2\file1.nxs"]

    output :
        ["C:\dir1\file1.nxs","C:\dir1\file2.nxs"]
    """
    files = [os.path.basename(full_path) for full_path in file_list]
    unique_files = [file_list[n] for n, file_name in enumerate(files) if file_name not in files[:n]]
    return unique_files


def parse_user_input_to_files(input_text, extensions=allowed_extensions):
    """
    Parse user input from load file widget into list of filenames.

    Example

    input_text = "C:\dir1\dir2\file1.nxs;C:\dir1\file2.nxs"

    output :
        ["file1.nxs", "file2.nxs"]
    """
    input_list = input_text.split(";")
    filenames = []
    for text in input_list:
        if os.path.splitext(text)[-1].lower() in ["." + ext for ext in extensions]:
            filenames += [text]
    return filenames
