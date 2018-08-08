from __future__ import (absolute_import, division, print_function)

allowed_instruments = ["EMU", "MUSR", "CHRONUS", "HIFI"]


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
        return ""


def get_current_run_filename(instrument):
    """
    If instrument is supported, attempts to find the file on the ISIS network which
    contains the data from its current (most up-to-date) run.
    """

    instrument_directory = get_instrument_directory(instrument)
    if instrument_directory is None:
        return ""

    autosave_file = "\\\\" + instrument_directory + "\\data\\autosave.run"
    autosave_points_to = ""
    with open(autosave_file, 'r') as f:
        for line in f:
            if len(line.split('.')) == 2:
                autosave_points_to = line
    if autosave_points_to == "":
        psudoDAE = "\\\\" + instrument_directory + "\\data\\" + instrument_directory + "auto_A.tmp";
    else:
        psudoDAE = "\\\\" + instrument_directory + "\\data\\" + autosave_points_to
    return psudoDAE


def format_run_for_file(run):
    return "{0:08d}".format(run)

def file_path_for_instrument_and_run(instrument, run):
    base_dir = "\\\\" +get_instrument_directory(instrument) + "\\data"
    file_name = instrument + format_run_for_file(run) + ".nxs"
    return base_dir.lower() + "\\" + file_name
