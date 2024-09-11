# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
"""The elements of this module coordinate file access and information extraction from files."""

# pylint: disable=too-few-public-methods, invalid-name

import os
import h5py as h5
import re
from abc import ABCMeta, abstractmethod
from mantid.api import FileFinder
from mantid.kernel import DateAndTime, ConfigService, Logger
from mantid.api import AlgorithmManager, ExperimentInfo
from sans.common.enums import SANSInstrument, FileType, SampleShape
from sans.common.general_functions import get_instrument, instrument_name_correction, get_facility

# ----------------------------------------------------------------------------------------------------------------------
# Constants
# ----------------------------------------------------------------------------------------------------------------------
# File extensions

NXS_EXTENSION = "nxs"
RAW_EXTENSION = "raw"
RAW_EXTENSION_WITH_DOT = ".RAW"

ADDED_SUFFIX = "-add_added_event_data"
ADDED_MONITOR_SUFFIX = "-add_monitors_added_event_data"
ADD_FILE_SUFFIX = "-ADD.NXS"

PARAMETERS_XML_SUFFIX = "_Parameters.xml"

# Nexus key words
RAW_DATA_1 = "raw_data_1"
PERIODS = "periods"
PROTON_CHARGE = "proton_charge"
INSTRUMENT = "instrument"
NAME = "name"
START_TIME = "start_time"
RUN_NUMBER = "run_number"
NX_CLASS = "NX_class"
NX_EVENT_DATA = "NXevent_data"
LOGS = "logs"
VALUE = "value"
WORKSPACE_NAME = "workspace_name"
END_TIME = "r_endtime"
END_DATE = "r_enddate"
MANTID_WORKSPACE_PREFIX = "mantid_workspace_"
EVENT_WORKSPACE = "event_workspace"

# Other
DEFINITION = "Definition"
PARAMETERS = "Parameters"

# Geometry
SAMPLE = "sample"
WIDTH = "width"
HEIGHT = "height"
THICKNESS = "thickness"
SHAPE = "shape"
CYLINDER = "CYLINDER"
FLAT_PLATE = "FLAT PLATE"
DISC = "DISC"
GEOM_HEIGHT = "geom_height"
GEOM_WIDTH = "geom_width"
GEOM_THICKNESS = "geom_thickness"
GEOM_ID = "geom_id"
E_HEIGHT = "e_height"
E_WIDTH = "e_width"
E_THICK = "e_thick"
E_GEOM = "e_geom"


# ----------------------------------------------------------------------------------------------------------------------
# General functions
# ----------------------------------------------------------------------------------------------------------------------
def find_full_file_path(file_name):
    """
    Gets the full path of a file name if it is available on the Mantid paths.

    :param file_name: the name of the file.
    :return: the full file path.
    """
    return FileFinder.getFullPath(file_name)


def find_sans_file(file_name):
    """
    Finds a SANS file.
    The file can be specified as:
    1. file.ext or  path1 path2 file.ext
    2. run number
    :param file_name: a file name or a run number.
    :return: the full path.
    """
    full_path = find_full_file_path(file_name)
    if not full_path and not file_name.endswith(".nxs"):
        full_path = find_full_file_path(file_name + ".nxs")
    if not full_path:
        # TODO: If we only provide a run number for example 98843 for LOQ measurments, but have LARMOR specified as the
        #       Mantid instrument, then the FileFinder will search itself to death. This is a general Mantid issue.
        #       One way to handle this graceful would be a timeout option.
        runs = FileFinder.findRuns(file_name)
        if runs:
            full_path = runs[0]

    if not full_path:
        error_message = (
            "Trying to find the SANS file {0}, but cannot find it. Make sure that "
            "the relevant paths are added and the correct instrument is selected."
        )
        raise RuntimeError(error_message.format(file_name))
    return full_path


def get_extension_for_file_type(file_info):
    """
    Get the extension for a specific file type.

    :param file_info: a SANSFileInformation object.
    :return: the extension a string. This can be either nxs or raw.
    """
    if file_info.get_type() is FileType.ISIS_NEXUS or file_info.get_type() is FileType.ISIS_NEXUS_ADDED:
        extension = NXS_EXTENSION
    elif file_info.get_type() is FileType.ISIS_RAW:
        extension = RAW_EXTENSION
    else:
        raise RuntimeError("The file extension type for a file of type {0} is unknown" "".format(str(file_info.get_type())))
    return extension


def get_number_of_periods(func, file_name):
    """
    Get the number of periods of the data in a file.

    :param func: a function handle which extracts the relevant information.
    :param file_name: the file name to the relevant file.
    :return: the number of periods if it is applicable else 0.
    """
    is_file_type, number_of_periods = func(file_name)
    return number_of_periods if is_file_type else 0


def is_single_period(func, file_name):
    """
    Checks if a file contains only single period data.

    :param func: a function handle which extracts the number of periods.
    :param file_name: the name of the file.
    :return: true if the number of periods is 1 else false.
    """
    is_file_type, number_of_periods = func(file_name)
    return is_file_type and number_of_periods == 1


def is_multi_period(func, file_name):
    """
    Checks if a file contains multi-period data.

    :param func: a function handle which extracts the number of periods.
    :param file_name: the name of the file.
    :return: true if the number of periods is larger than one else false.
    """
    is_file_type, number_of_periods = func(file_name)
    return is_file_type and number_of_periods >= 1


def get_instrument_paths_for_sans_file(file_name=None, file_information=None):
    """
    Gets the Instrument Definition File (IDF) path and the Instrument Parameter Path (IPF) path associated with a file.

    :param file_name: the file name is a name fo a SANS data file, e.g. SANS2D0001234. This or the file_information
                      has to be specified.
    :param file_information: a file_information object. either this or the file_name has to be specified.
    :return: the IDF path and the IPF path
    """

    def get_file_location(path):
        return os.path.dirname(path)

    def get_ipf_equivalent_name(path):
        # If XXX_Definition_Yyy.xml is the IDF name, then the equivalent  IPF name is: XXX_Parameters_Yyy.xml
        base_file_name = os.path.basename(path)
        return base_file_name.replace(DEFINITION, PARAMETERS)

    def get_ipf_standard_name(path):
        # If XXX_Definition_Yyy.xml is the IDF name, then the standard IPF name is: XXX_Parameters.xml
        base_file_name = os.path.basename(path)
        elements = base_file_name.split("_")
        return elements[0] + PARAMETERS_XML_SUFFIX

    def check_for_files(directory, path):
        # Check if XXX_Parameters_Yyy.xml exists in the same folder
        ipf_equivalent_name = get_ipf_equivalent_name(path)
        ipf_equivalent = os.path.join(directory, ipf_equivalent_name)
        if os.path.exists(ipf_equivalent):
            return ipf_equivalent

        # Check if XXX_Parameters.xml exists in the same folder
        ipf_standard_name = get_ipf_standard_name(path)
        ipf_standard = os.path.join(directory, ipf_standard_name)
        if os.path.exists(ipf_standard):
            return ipf_standard
        # Does not seem to be in the folder
        return None

    def get_ipf_for_rule_1(path):
        # Check if can be found in the same folder
        directory = get_file_location(path)
        return check_for_files(directory, path)

    def get_ipf_for_rule_2(path):
        # Check if can be found in the instrument folder
        directory = ConfigService.getInstrumentDirectory()
        return check_for_files(directory, path)

    # Get the measurement date
    if not isinstance(file_information, SANSFileInformation):
        file_information_factory = SANSFileInformationFactory()
        file_information = file_information_factory.create_sans_file_information(file_name)
    measurement_time = file_information.get_date()

    # For some odd reason the __str__ method of DateAndTime adds a space which we need to strip here. It seems
    # to be on purpose though since the export method is called IS08601StringPlusSpace --> hence we need to strip it
    # ourselves
    measurement_time_as_string = str(measurement_time).strip()

    # Get the instrument
    instrument = file_information.get_instrument()
    instrument_as_string = instrument.value

    # Get the idf file path
    # IMPORTANT NOTE: I profiled the call to ExperimentInfo.getInstrumentFilename and it dominates
    #                 the state creation. Ironically this routine is exported from C++. The problem is
    #                 that we are performing XML parsing on the C++ side, which is costly. There is a
    #                 movement currently towards making the IDF redundant and storing instrument info
    #                 as native nexus information.
    # TODO for optimization: Add the IDF path to a global cache layer which takes the
    #                        instrument name and the from-to dates
    idf_path = ExperimentInfo.getInstrumentFilename(instrument_as_string, measurement_time_as_string)
    idf_path = os.path.normpath(idf_path)

    if not os.path.exists(idf_path):
        raise RuntimeError("SANSFileInformation: The instrument definition file {0} does not seem to " "exist.".format(str(idf_path)))

    # Get the ipf path. This is slightly more complicated. See the Mantid documentation for the naming rules. Currently
    # they are:
    # 1. If the IDF is not in the instrument folder and there is another X_Parameters.xml in the same folder,
    #    this one in the same folder will be used instead of any parameter file in the instrument folder.
    # 2. If you want one parameter file for your IDF file, name your IDF file X_Definition_Yyy.xml and the parameter
    #    file X_Parameters_Yyy.xml , where Yyy is any combination a characters you find appropriate. If your IDF
    #    file is not in the instrument folder, the parameter file can be in either the same folder or in the instrument
    #    folder, but it can only be in the instrument folder, if the same folder has no X_Parameters.xml or
    #    X_Parameters_Yyy.xml file. If there is no X_Parameters_Yyy.xml file, X_Parameters.xml would be used.
    ipf_rule1 = get_ipf_for_rule_1(idf_path)
    if ipf_rule1:
        return idf_path, ipf_rule1

    ipf_rule2 = get_ipf_for_rule_2(idf_path)
    if ipf_rule2:
        return idf_path, ipf_rule2

    raise RuntimeError(
        "SANSFileInformation: There does not seem to be a corresponding instrument parameter file " "available for {0}".format(
            str(idf_path)
        )
    )


def convert_to_shape(shape_flag):
    """
    Converts a shape flag to a shape object.

    :param shape_flag: a geometry flag which can be 1, 2 or 3
    :return: a shape object
    """
    if shape_flag == 1:
        shape = SampleShape.CYLINDER
    elif shape_flag == 2:
        shape = SampleShape.FLAT_PLATE
    elif shape_flag == 3:
        shape = SampleShape.DISC
    else:
        shape = None
    return shape


def convert_to_flag(shape_string):
    """
    Converts a shape flag to a shape object.

    :param shape_flag: a geometry flag which can be 1, 2 or 3
    :return: a shape object
    """
    if shape_string == SampleShape.CYLINDER:
        shape = 1
    elif shape_string == SampleShape.FLAT_PLATE:
        shape = 2
    elif shape_string == SampleShape.DISC:
        shape = 3
    else:
        shape = None
    return shape


# ----------------------------------------------------------------------------------------------------------------------
# Functions for ISIS Nexus
# ----------------------------------------------------------------------------------------------------------------------
def get_isis_nexus_info(file_name):
    """
    Get information if is ISIS Nexus and the number of periods.

    :param file_name: the full file path.
    :return: if the file was a Nexus file and the number of periods.
    """
    try:
        with h5.File(file_name, "r") as h5_file:
            keys = list(h5_file.keys())
            is_isis_nexus = RAW_DATA_1 in keys
            if is_isis_nexus:
                first_entry = h5_file[RAW_DATA_1]
                period_group = first_entry[PERIODS]
                proton_charge_data_set = period_group[PROTON_CHARGE]
                number_of_periods = len(proton_charge_data_set)
            else:
                number_of_periods = -1
    except IOError:
        is_isis_nexus = False
        number_of_periods = -1
    return is_isis_nexus, number_of_periods


def is_isis_nexus_single_period(file_name):
    return is_single_period(get_isis_nexus_info, file_name)


def is_isis_nexus_multi_period(file_name):
    return is_multi_period(get_isis_nexus_info, file_name)


def get_number_of_periods_for_isis_nexus(file_name):
    return get_number_of_periods(get_isis_nexus_info, file_name)


def get_instrument_name_for_isis_nexus(file_name):
    """
    Instrument information is
    file|
        |--mantid_workspace_1/raw_data_1|
                                        |--instrument|
                                                     |--name
    """
    with h5.File(file_name, "r") as h5_file:
        # Open first entry
        keys = list(h5_file.keys())
        first_entry = h5_file[keys[0]]
        # Open instrument group
        instrument_group = first_entry[INSTRUMENT]
        # Open name data set
        name_data_set = instrument_group[NAME]
        # Read value
        instrument_name = name_data_set[0].decode("utf-8")
    return instrument_name


def get_top_level_nexus_entry(file_name, entry_name):
    """
    Gets the first entry in a Nexus file.

    :param file_name: The file name
    :param entry_name: the entry name
    :return:
    """
    with h5.File(file_name, "r") as h5_file:
        # Open first entry
        keys = list(h5_file.keys())
        top_level = h5_file[keys[0]]
        entry = top_level[entry_name]
        value = entry[0]
    return value


def get_date_for_isis_nexus(file_name):
    value = get_top_level_nexus_entry(file_name, START_TIME)
    return DateAndTime(value)


def is_raw_nexus_event_mode(file_name):
    """
    Event mode files have a class with a "NXevent_data" type
    Structure:
    |--mantid_workspace_1/raw_data_1|
                                    |--some_group|
                                                 |--Attribute: NX_class = NXevent_data
    """
    with h5.File(file_name, "r") as h5_file:
        # Open first entry
        keys = list(h5_file.keys())
        first_entry = h5_file[keys[0]]
        # Open instrument group
        is_event_mode = False
        for value in list(first_entry.values()):
            if NX_CLASS in value.attrs and NX_EVENT_DATA == value.attrs[NX_CLASS].decode("utf-8"):
                is_event_mode = True
                break
    return is_event_mode


def get_geometry_information_isis_nexus(file_name):
    """
    Gets geometry information from the sample folder in the nexus file

    :param file_name:
    :return: height, width, thickness, shape
    """
    with h5.File(file_name, "r") as h5_file:
        # Open first entry
        keys = list(h5_file.keys())
        top_level = h5_file[keys[0]]
        sample = top_level[SAMPLE]
        height = float(sample[HEIGHT][0])
        width = float(sample[WIDTH][0])
        thickness = float(sample[THICKNESS][0])
        shape_as_string = sample[SHAPE][0].upper().decode("utf-8")
        if shape_as_string == CYLINDER:
            shape = SampleShape.CYLINDER
        elif shape_as_string == FLAT_PLATE:
            shape = SampleShape.FLAT_PLATE
        elif shape_as_string == DISC:
            shape = SampleShape.DISC
        else:
            shape = None
    return height, width, thickness, shape


# ----------------------------------------------------------------------------------------------------------------------
# Functions for Added data
# ----------------------------------------------------------------------------------------------------------------------
# 1. An added file will always have a file name SOME_NAME-add.nxs, ie we can preselect an added file by the -add
#    qualifier
# 2. Scenario 1: Added histogram data, ie files which were added and saved as histogram data.
# 2.1 Added histogram files will contain one or more (if they were based on multi-period files) entries in the hdfd5
#    file where the first level entry will be named mantid_workspace_X where X=1,2,3,... . Note that the numbers
#    correspond  to periods.
# 3. Scenario 2: Added event data, ie files which were added and saved as event data.


def check_nexus_information(file_name):
    """
    Get information if is added data and the number of periods.

    :param file_name: the full file path.
    :return: if the file was a Nexus file and the number of periods.
    """
    ADDED_SUFFIX = "_added_event_data"
    ADDED_MONITOR_SUFFIX = "_monitors_added_event_data"

    def get_all_keys_for_top_level(key_collection):
        top_level_key_collection = []
        for key in key_collection:
            if key.startswith("mantid_workspace_"):
                top_level_key_collection.append(key)
        return sorted(top_level_key_collection)

    with h5.File(file_name, "r") as h5_file:
        # Get all mantid_workspace_X keys
        keys = list(h5_file.keys())
        top_level_keys = get_all_keys_for_top_level(keys)
        # This magic workspace name gets used whenever Mantid has touched a file
        # so we will assume anything we've touched is an add file.
        # If this assumption no longer holds true, you are welcome to find another
        # fingerprint and complain at me, since this was the "easiest" method
        nexus_added_tag_present = any("mantid_workspace" in key for key in top_level_keys)

    def check_if_event_mode(entry):
        return "event_workspace" in list(entry.keys())

    def get_workspace_name(entry, file_name):
        return entry[WORKSPACE_NAME][0].decode("utf-8") if WORKSPACE_NAME in entry else file_name

    def has_same_number_of_entries(workspace_names, monitor_workspace_names):
        return len(workspace_names) == len(monitor_workspace_names)

    def entries_match(workspace_names, monitor_workspace_names):
        altered_names = [ws_name.replace(ADDED_SUFFIX, ADDED_MONITOR_SUFFIX) for ws_name in workspace_names]
        return all([ws_name in monitor_workspace_names for ws_name in altered_names])

    def get_added_event_info(h5_file_handle, key_collection, file_name):
        """
        We expect to find one event workspace and one histogram workspace per period
        """
        workspace_names = []
        monitor_workspace_names = []
        for key in key_collection:
            entry = h5_file_handle[key]
            is_event_mode = check_if_event_mode(entry)
            workspace_name = get_workspace_name(entry, file_name)
            if is_event_mode:
                workspace_names.append(workspace_name)
            else:
                monitor_workspace_names.append(workspace_name)

        # There are several criteria which need to be full filled to be sure that we are dealing with added event data
        # 1. There have to be the same number of event and non-event entries, since your each data set we have a
        #    monitor data set.
        # 2. Every data entry needs to have "-ADD_ADDED_EVENT_DATA" in the workspace name and every
        #    monitor data entry needs to have a "ADD_MONITORS_ADDED_EVENT_DATA" in the workspace name.
        # 3. Every data entry has matching monitor entry, e.g. random_name-add_added_event_data_4 needs
        #    random_name-add_monitors_added_event_data_4.s
        if has_same_number_of_entries(workspace_names, monitor_workspace_names) and entries_match(workspace_names, monitor_workspace_names):
            is_added_file_event = True
            num_periods = len(workspace_names)
        else:
            is_added_file_event = False
            num_periods = 1

        return is_added_file_event, num_periods

    def get_added_histogram_info(h5_file_handle, key_collection):
        # We only have to make sure that all entries are non-event type
        is_added_file_histogram = True
        num_periods = len(key_collection)
        for key in key_collection:
            entry = h5_file_handle[key]
            if check_if_event_mode(entry):
                is_added_file_histogram = False
                num_periods = 1
                break
        return is_added_file_histogram, num_periods

    if not nexus_added_tag_present:
        is_event = is_raw_nexus_event_mode(file_name)
        number_of_periods = 1
        return nexus_added_tag_present, number_of_periods, is_event

    with h5.File(file_name, "r") as h5_file:
        # Get all mantid_workspace_X keys
        # Check if entries are added event data, if we don't have a hit, then it can always be
        # added histogram data
        is_added_event_file, number_of_periods_event = get_added_event_info(h5_file, top_level_keys, file_name)
        is_added_histogram_file, number_of_periods_histogram = get_added_histogram_info(h5_file, top_level_keys)

        number_of_periods = number_of_periods_event

        if is_added_event_file:
            is_event = True
            number_of_periods = number_of_periods_event
        elif is_added_histogram_file:
            is_event = False
            number_of_periods = number_of_periods_histogram
        else:
            raise RuntimeError("Ended up in added branch where it's neither processed or raw?")

    return nexus_added_tag_present, number_of_periods, is_event


def get_date_for_added_workspace(file_name):
    value = get_top_level_nexus_entry(file_name, "start_time")
    return DateAndTime(value)


def has_added_suffix(file_name):
    suffix = "-ADD.NXS"
    return file_name.upper().endswith(suffix)


def is_added_histogram(file_name):
    is_added, _, is_event = check_nexus_information(file_name)
    return is_added and not is_event


def is_added_event(file_name):
    is_added, _, is_event = check_nexus_information(file_name)
    return is_added and is_event


def get_geometry_information_isis_added_nexus(file_name):
    """
    Gets geometry information from the sample folder in an added nexus file

    :param file_name: the file name
    :return: height, width, thickness, shape
    """
    with h5.File(file_name, "r") as h5_file:
        # Open first entry
        keys = list(h5_file.keys())
        top_level = h5_file[keys[0]]
        sample = top_level[SAMPLE]
        height = float(sample[GEOM_HEIGHT][0])
        width = float(sample[GEOM_WIDTH][0])
        thickness = float(sample[GEOM_THICKNESS][0])
        shape_id = int(sample[GEOM_ID][0])
        shape = convert_to_shape(shape_id)
    return height, width, thickness, shape


# ----------------------------------------------------------------------------------------------------------------------
# ISIS Raw
# ----------------------------------------------------------------------------------------------------------------------
def get_raw_info(file_name):
    # Preselect files which don't end with .raw
    split_file_name, file_extension = os.path.splitext(file_name)
    if file_extension.upper() != RAW_EXTENSION_WITH_DOT:
        is_raw = False
        number_of_periods = -1
    else:
        try:
            alg_info = AlgorithmManager.createUnmanaged("RawFileInfo")
            alg_info.initialize()
            alg_info.setChild(True)
            alg_info.setProperty("Filename", file_name)
            alg_info.setProperty("GetRunParameters", True)
            alg_info.execute()

            periods = alg_info.getProperty("PeriodCount").value
            is_raw = True
            number_of_periods = periods
        except IOError:
            is_raw = False
            number_of_periods = -1

    return is_raw, number_of_periods


def is_raw_single_period(file_name):
    return is_single_period(get_raw_info, file_name)


def is_raw_multi_period(file_name):
    return is_multi_period(get_raw_info, file_name)


def get_from_raw_header(file_name, index):
    alg_info = AlgorithmManager.createUnmanaged("RawFileInfo")
    alg_info.initialize()
    alg_info.setChild(True)
    alg_info.setProperty("Filename", file_name)
    alg_info.setProperty("GetRunParameters", True)
    alg_info.execute()

    header = alg_info.getProperty("RunHeader").value
    element = header.split()[index]
    return element


def get_instrument_name_for_raw(file_name):
    instrument_name = get_from_raw_header(file_name, 0)
    # We sometimes need an instrument name correction, eg SANS2D is sometimes stored as SAN
    return instrument_name_correction(instrument_name)


def get_number_of_periods_for_raw(file_name):
    return get_number_of_periods(get_raw_info, file_name)


def get_date_for_raw(file_name):
    def get_month(month_string):
        month_conversion = {
            "JAN": "01",
            "FEB": "02",
            "MAR": "03",
            "APR": "04",
            "MAY": "05",
            "JUN": "06",
            "JUL": "07",
            "AUG": "08",
            "SEP": "09",
            "OCT": "10",
            "NOV": "11",
            "DEC": "12",
        }
        month_upper = month_string.upper()
        if month_upper in month_conversion:
            return month_conversion[month_upper]
        else:
            raise RuntimeError("Cannot get measurement time. Invalid month in Raw file: " + month_upper)

    def get_raw_measurement_time(date_input, time_input):
        year = date_input[7 : (7 + 4)]
        day = date_input[0:2]
        month_string = date_input[3:6]
        month = get_month(month_string)

        date_and_time_string = year + "-" + month + "-" + day + "T" + time_input
        return DateAndTime(date_and_time_string)

    alg_info = AlgorithmManager.createUnmanaged("RawFileInfo")
    alg_info.initialize()
    alg_info.setChild(True)
    alg_info.setProperty("Filename", file_name)
    alg_info.setProperty("GetRunParameters", True)
    alg_info.execute()

    run_parameters = alg_info.getProperty("RunParameterTable").value

    keys = run_parameters.getColumnNames()

    time_id = END_TIME
    date_id = END_DATE

    time = run_parameters.column(keys.index(time_id))
    date = run_parameters.column(keys.index(date_id))
    time = time[0]
    date = date[0]
    return get_raw_measurement_time(date, time)


def get_geometry_information_raw(file_name):
    """
    Gets the geometry information form the table workspace with the spb information

    :param file_name: the full file name to an existing raw file.
    :return: height, width, thickness and shape
    """
    alg_info = AlgorithmManager.createUnmanaged("RawFileInfo")
    alg_info.initialize()
    alg_info.setChild(True)
    alg_info.setProperty("Filename", file_name)
    alg_info.setProperty("GetRunParameters", False)
    alg_info.setProperty("GetSampleParameters", True)
    alg_info.execute()

    sample_parameters = alg_info.getProperty("SampleParameterTable").value
    keys = sample_parameters.getColumnNames()

    height_id = E_HEIGHT
    width_id = E_WIDTH
    thickness_id = E_THICK
    shape_id = E_GEOM

    height = sample_parameters.column(keys.index(height_id))[0]
    width = sample_parameters.column(keys.index(width_id))[0]
    thickness = sample_parameters.column(keys.index(thickness_id))[0]
    shape_flag = sample_parameters.column(keys.index(shape_id))[0]
    shape = convert_to_shape(shape_flag)
    return height, width, thickness, shape


# ----------------------------------------------------------------------------------------------------------------------
# SANS file Information
# ----------------------------------------------------------------------------------------------------------------------
class SANSFileInformation(metaclass=ABCMeta):
    logger = Logger("SANS")

    def __init__(self, full_file_name):
        self._full_file_name = full_file_name

        # Idf and Ipf file path (will be loaded via lazy evaluation)
        self._idf_file_path = None
        self._ipf_file_path = None

        self._run_number = self._init_run_number()

    def __eq__(self, other):
        if type(other) is type(self):
            return self.__dict__ == other.__dict__
        return False

    @abstractmethod
    def get_file_name(self):
        pass

    @abstractmethod
    def get_instrument(self):
        pass

    @abstractmethod
    def get_facility(self):
        pass

    @abstractmethod
    def get_date(self):
        pass

    @abstractmethod
    def get_number_of_periods(self):
        pass

    @abstractmethod
    def get_type(self):
        pass

    @abstractmethod
    def is_event_mode(self):
        pass

    @abstractmethod
    def is_added_data(self):
        pass

    @abstractmethod
    def get_height(self):
        pass

    @abstractmethod
    def get_width(self):
        pass

    @abstractmethod
    def get_thickness(self):
        pass

    @abstractmethod
    def get_shape(self):
        pass

    def get_run_number(self):
        return self._run_number

    @abstractmethod
    def _get_run_number_from_file(self, file_name):
        pass

    def _init_run_number(self):
        # We don't use the nexus tagged file name as some instruments will take a file with
        # the "right structure" and transplant data from another in, then rename the recipient to the donor name

        run_filename = os.path.basename(self._full_file_name)

        # Split down all digits into separate groups
        run_number_list = re.findall(r"\d+", run_filename)
        # Filter out any single digit numbers, such as SANS-2-Dxxxx
        run_number_list = [run_number for run_number in run_number_list if len(run_number) > 1]

        # Assume run number is largest value in the list
        run_number = max(run_number_list) if run_number_list else None

        if not run_number:
            run_number = self._get_run_number_from_file(self._full_file_name)
            self.logger.warning(
                "Could not parse run number from filename, using the run number direct set in the file which is {0}".format(run_number)
            )

        return int(run_number)

    def get_idf_file_path(self):
        if self._idf_file_path is None:
            idf_path, ipf_path = get_instrument_paths_for_sans_file(file_information=self)
            self._idf_file_path = idf_path
            self._ipf_file_path = ipf_path
        return self._idf_file_path

    def get_ipf_file_path(self):
        if self._ipf_file_path is None:
            idf_path, ipf_path = get_instrument_paths_for_sans_file(file_information=self)
            self._idf_file_path = idf_path
            self._ipf_file_path = ipf_path
        return self._ipf_file_path

    @staticmethod
    def get_full_file_name(file_name):
        return find_sans_file(file_name)


class SANSFileInformationBlank(SANSFileInformation):
    """
    Blank SANS File information to avoid mocks being called in, in the future this should be removed
    as we should not be creating blank information states
    """

    def __init__(self):
        super(SANSFileInformationBlank, self).__init__(full_file_name="00000")

    def get_file_name(self):
        raise NotImplementedError("Trying to use blank FileInformation")

    def get_instrument(self):
        raise NotImplementedError("Trying to use blank FileInformation")

    def get_facility(self):
        raise NotImplementedError("Trying to use blank FileInformation")

    def get_date(self):
        raise NotImplementedError("Trying to use blank FileInformation")

    def get_number_of_periods(self):
        raise NotImplementedError("Trying to use blank FileInformation")

    def get_type(self):
        raise NotImplementedError("Trying to use blank FileInformation")

    def is_event_mode(self):
        raise NotImplementedError("Trying to use blank FileInformation")

    def is_added_data(self):
        raise NotImplementedError("Trying to use blank FileInformation")

    def get_height(self):
        raise NotImplementedError("Trying to use blank FileInformation")

    def get_width(self):
        raise NotImplementedError("Trying to use blank FileInformation")

    def get_thickness(self):
        raise NotImplementedError("Trying to use blank FileInformation")

    def get_shape(self):
        raise NotImplementedError("Trying to use blank FileInformation")

    def _get_run_number_from_file(self, file_name):
        raise NotImplementedError("Trying to use blank FileInformation")


class SANSFileInformationISISNexus(SANSFileInformation):
    def __init__(self, file_name, is_event):
        super(SANSFileInformationISISNexus, self).__init__(file_name)
        # Setup instrument name
        instrument_name = get_instrument_name_for_isis_nexus(self._full_file_name)
        self._instrument = SANSInstrument[instrument_name]

        # Setup the facility
        self._facility = get_facility(self._instrument)

        # Setup date
        self._date = get_date_for_isis_nexus(self._full_file_name)

        # Setup number of periods
        self._number_of_periods = get_number_of_periods_for_isis_nexus(self._full_file_name)
        self._is_event_mode = is_event

        # Get geometry details
        height, width, thickness, shape = get_geometry_information_isis_nexus(self._full_file_name)
        self._height = height if height is not None else 1.0
        self._width = width if width is not None else 1.0
        self._thickness = thickness if thickness is not None else 1.0
        self._shape = shape if shape is not None else SampleShape.DISC

    def get_file_name(self):
        return self._full_file_name

    def get_instrument(self):
        return self._instrument

    def get_facility(self):
        return self._facility

    def get_date(self):
        return self._date

    def get_number_of_periods(self):
        return self._number_of_periods

    def get_type(self):
        return FileType.ISIS_NEXUS

    def is_event_mode(self):
        return self._is_event_mode

    def is_added_data(self):
        return False

    def get_height(self):
        return self._height

    def get_width(self):
        return self._width

    def get_thickness(self):
        return self._thickness

    def get_shape(self):
        return self._shape

    def _get_run_number_from_file(self, file_name):
        return int(get_top_level_nexus_entry(file_name, RUN_NUMBER))


class SANSFileInformationISISAdded(SANSFileInformation):
    def __init__(self, file_name, num_periods, is_event):
        super(SANSFileInformationISISAdded, self).__init__(file_name)
        # Setup instrument name
        instrument_name = get_instrument_name_for_isis_nexus(self._full_file_name)
        self._instrument = get_instrument(instrument_name)

        # Setup the facility
        self._facility = get_facility(self._instrument)

        self._date, _ = self._get_date_and_run_number_added_nexus(self._full_file_name)
        self._number_of_periods = num_periods
        self._is_event_mode = is_event

        # Get geometry details
        height, width, thickness, shape = get_geometry_information_isis_added_nexus(self._full_file_name)
        self._height = height if height is not None else 1.0
        self._width = width if width is not None else 1.0
        self._thickness = thickness if thickness is not None else 1.0
        self._shape = shape if shape is not None else SampleShape.DISC

    def get_file_name(self):
        return self._full_file_name

    def get_instrument(self):
        return self._instrument

    def get_facility(self):
        return self._facility

    def get_date(self):
        return self._date

    def get_number_of_periods(self):
        return self._number_of_periods

    def get_type(self):
        return FileType.ISIS_NEXUS_ADDED

    def is_event_mode(self):
        return self._is_event_mode

    def is_added_data(self):
        return True

    def get_height(self):
        return self._height

    def get_width(self):
        return self._width

    def get_thickness(self):
        return self._thickness

    def get_shape(self):
        return self._shape

    def _get_run_number_from_file(self, file_name):
        _, run_number = self._get_date_and_run_number_added_nexus(file_name)
        return run_number

    @staticmethod
    def _get_date_and_run_number_added_nexus(file_name):
        with h5.File(file_name, "r") as h5_file:
            keys = list(h5_file.keys())
            first_entry = h5_file[keys[0]]
            logs = first_entry["logs"]
            # Start time
            start_time = logs["start_time"]
            start_time_value = DateAndTime(start_time["value"][0])
            # Run number
            run_number = logs["run_number"]
            run_number_value = int(run_number["value"][0])
        return start_time_value, run_number_value


class SANSFileInformationRaw(SANSFileInformation):
    def __init__(self, file_name):
        super(SANSFileInformationRaw, self).__init__(file_name)
        # Setup instrument name
        instrument_name = get_instrument_name_for_raw(self._full_file_name)
        self._instrument = SANSInstrument[instrument_name]

        # Setup the facility
        self._facility = get_facility(self._instrument)

        # Setup date
        self._date = get_date_for_raw(self._full_file_name)

        # Setup number of periods
        self._number_of_periods = get_number_of_periods_for_raw(self._full_file_name)

        # Set geometry
        # Raw files don't have the sample information, so set to default
        height, width, thickness, shape = get_geometry_information_raw(self._full_file_name)
        self._height = height if height is not None else 1.0
        self._width = width if width is not None else 1.0
        self._thickness = thickness if thickness is not None else 1.0
        self._shape = shape if shape is not None else SampleShape.DISC

    def get_file_name(self):
        return self._full_file_name

    def get_instrument(self):
        return self._instrument

    def get_facility(self):
        return self._facility

    def get_date(self):
        return self._date

    def get_number_of_periods(self):
        return self._number_of_periods

    def get_type(self):
        return FileType.ISIS_RAW

    def is_event_mode(self):
        return False

    def is_added_data(self):
        return False

    def get_height(self):
        return self._height

    def get_width(self):
        return self._width

    def get_thickness(self):
        return self._thickness

    def get_shape(self):
        return self._shape

    def _get_run_number_from_file(self, file_name):
        return int(get_from_raw_header(file_name, 1))


class SANSFileInformationFactory(object):
    def __init__(self):
        super(SANSFileInformationFactory, self).__init__()

    def create_sans_file_information(self, file_name):
        if not file_name:
            raise ValueError("The filename given to FileInformation is empty")

        full_file_name = find_sans_file(file_name)
        if is_raw_single_period(full_file_name) or is_raw_multi_period(full_file_name):
            return SANSFileInformationRaw(full_file_name)

        # We should have a nexus file, let's figure out what it is
        is_added, number_of_periods, is_event = check_nexus_information(full_file_name)

        if is_added:
            file_information = SANSFileInformationISISAdded(full_file_name, number_of_periods, is_event)
        elif is_event or is_isis_nexus_single_period(full_file_name) or is_isis_nexus_multi_period(full_file_name):
            assert not is_added, "Order has been changed, an added file should not use this loader"
            file_information = SANSFileInformationISISNexus(full_file_name, is_event)
        else:
            raise NotImplementedError("The file type you have provided is not implemented yet.")
        return file_information
