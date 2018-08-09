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


# Allow any filepath, only allow .nxs files separated by commas
def parse_input_files(text):
    files = text.split(",")
    for file in files:
        try:
            filestem = file.split(".")[-1]
        except:
            raise ValueError("Please supply a valid list of files")

        if filestem != "nxs":
            raise ValueError("Only .nxs files supported")
    return files


# Fill in digits to make num2 the same as num1
def fill_digits(num1, num2):
    extraDigits = len(str(num1)) - len(str(num2))
    return int(str(num1)[0:extraDigits] + str(num2))


def parse_range_string(text):
    rangeList = text.split("-")
    if len(rangeList) == 1:
        return [int(rangeList[0])]
    if len(rangeList) == 2:
        minRange = int(rangeList[0])
        maxRange = int(rangeList[1])
        if minRange > maxRange:
            # Assume the second number is truncated
            maxRange = fill_digits(minRange, maxRange)
        if maxRange - minRange > 500:
            raise ValueError("Range from " + str(minRange) + " to " + str(maxRange) + " is too large")
        return [minRange + i for i in range(maxRange - minRange + 1)]


# return ordered and unique.
def parse_run_string(text):
    runs = []
    ranges = text.split(",")
    for runRange in ranges:
        runs += parse_range_string(runRange)
    return list(set(runs))


def parse_run_list_to_filenames(runList, instrument):
    files = []
    for run in runList:
        # pad to 8 zeros
        files += [instrument + '{:08d}'.format(run) + '.nxs']
    return files

    # print(fill_digits(1234, 35))
    # print(parse_range_string("1234-1245"))
    # print(parse_range_string("1234-45"))
    # print(parse_range_string("1234-9"))
    #
    # print(parse_run_string("1,2,3,4-10"))
    #
    # print(parse_run_string("1,2,3,4-10"))
    #
    # print(parse_run_string("26,27,1-25,6"))
    #
    # runList = parse_run_string("1234-1250")
    # print(parse_run_list_to_filenames(runList, "EMU"))