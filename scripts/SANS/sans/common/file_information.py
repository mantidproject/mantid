""" The elements of this module coordinate file access and information extraction from files."""

# pylint: disable=too-few-public-methods, invalid-name

from __future__ import (absolute_import, division, print_function)
import os
import h5py as h5
from abc import (ABCMeta, abstractmethod)

from mantid.api import FileFinder
from mantid.kernel import (DateAndTime, ConfigService)
from mantid.api import (AlgorithmManager, ExperimentInfo)
from sans.common.enums import (SANSInstrument, FileType)

from six import with_metaclass


# -----------------------------------
# Free Functions
# -----------------------------------
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
    if not full_path:
        runs = FileFinder.findRuns(file_name)
        if runs:
            full_path = runs[0]
    if not full_path:
        raise RuntimeError("Trying to find the SANS file {0}, but cannot find it. Make sure that "
                           "the relevant paths are added.".format(file_name))
    return full_path


def get_extension_for_file_type(file_info):
    """
    Get the extension for a specific file type.

    :param file_info: a SANSFileInformation object.
    :return: the extension a stirng. This can be either nxs or raw.
    """
    if file_info.get_type() is FileType.ISISNexus or file_info.get_type() is FileType.ISISNexusAdded:
        extension = "nxs"
    elif file_info.get_type() is FileType.ISISRaw:
        extension = "raw"
    else:
        raise RuntimeError("The file extension type for a file of type {0} is unknown"
                           "".format(str(file_info.get_type())))
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


def get_instrument_paths_for_sans_file(file_name):
    """
    Gets the Instrument Definition File (IDF) path and the Instrument Parameter Path (IPF) path associated with a file.

    :param file_name: the file name is a name fo a SANS data file, e.g. SANS2D0001234
    :return: the IDF path and the IPF path
    """
    def get_file_location(path):
        return os.path.dirname(path)

    def get_ipf_equivalent_name(path):
        # If XXX_Definition_Yyy.xml is the IDF name, then the equivalent  IPF name is: XXX_Parameters_Yyy.xml
        base_file_name = os.path.basename(path)
        return base_file_name.replace("Definition", "Parameters")

    def get_ipf_standard_name(path):
        # If XXX_Definition_Yyy.xml is the IDF name, then the standard IPF name is: XXX_Parameters.xml
        base_file_name = os.path.basename(path)
        elements = base_file_name.split("_")
        return elements[0] + "_Parameters.xml"

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
    file_information_factory = SANSFileInformationFactory()
    file_information = file_information_factory.create_sans_file_information(file_name)
    measurement_time = file_information.get_date()
    # For some odd reason the __str__ method of DateAndTime adds a space which we need to strip here. It seems
    # to be on purpose though since the export method is called IS08601StringPlusSpace --> hence we need to strip it
    # ourselves
    measurement_time_as_string = str(measurement_time).strip()

    # Get the instrument
    instrument = file_information.get_instrument()
    instrument_as_string = SANSInstrument.to_string(instrument)

    # Get the idf file path
    idf_path = ExperimentInfo.getInstrumentFilename(instrument_as_string, measurement_time_as_string)
    idf_path = os.path.normpath(idf_path)

    if not os.path.exists(idf_path):
        raise RuntimeError("SANSFileInformation: The instrument definition file {0} does not seem to "
                           "exist.".format(str(idf_path)))

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

    raise RuntimeError("SANSFileInformation: There does not seem to be a corresponding instrument parameter file "
                       "available for {0}".format(str(idf_path)))


# ----------------------------------------------
# Methods for ISIS Nexus
# ---------------------------------------------
def get_isis_nexus_info(file_name):
    """
    Get information if is ISIS Nexus and the number of periods.

    :param file_name: the full file path.
    :return: if the file was a Nexus file and the number of periods.
    """
    try:
        with h5.File(file_name) as h5_file:
            keys = list(h5_file.keys())
            is_isis_nexus = "raw_data_1" in keys
            first_entry = h5_file["raw_data_1"]
            period_group = first_entry["periods"]
            proton_charge_data_set = period_group["proton_charge"]
            number_of_periods = len(proton_charge_data_set)
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
    Instrument inforamtion is
    file|
        |--mantid_workspace_1/raw_data_1|
                                        |--instrument|
                                                     |--name
    """
    with h5.File(file_name) as h5_file:
        # Open first entry
        keys = list(h5_file.keys())
        first_entry = h5_file[keys[0]]
        # Open instrument group
        instrument_group = first_entry["instrument"]
        # Open name data set
        name_data_set = instrument_group["name"]
        # Read value
        instrument_name = name_data_set[0]
    return instrument_name


def get_top_level_nexus_entry(file_name, entry_name):
    """
    Gets the first entry in a Nexus file.

    :param file_name: The file name
    :param entry_name: the entry name
    :return:
    """
    with h5.File(file_name) as h5_file:
        # Open first entry
        keys = list(h5_file.keys())
        top_level = h5_file[keys[0]]
        entry = top_level[entry_name]
        value = entry[0]
    return value


def get_date_for_isis_nexus(file_name):
    value = get_top_level_nexus_entry(file_name, "start_time")
    return DateAndTime(value)


def get_run_number_for_isis_nexus(file_name):
    return int(get_top_level_nexus_entry(file_name, "run_number"))


def get_event_mode_information(file_name):
    """
    Event mode files have a class with a "NXevent_data" type
    Structure:
    |--mantid_workspace_1/raw_data_1|
                                    |--some_group|
                                                 |--Attribute: NX_class = NXevent_data
    """
    with h5.File(file_name) as h5_file:
        # Open first entry
        keys = list(h5_file.keys())
        first_entry = h5_file[keys[0]]
        # Open instrument group
        is_event_mode = False
        for value in list(first_entry.values()):
            if "NX_class" in value.attrs and "NXevent_data" == value.attrs["NX_class"]:
                is_event_mode = True
                break
    return is_event_mode


# ---------
# ISIS Raw
# ---------
def get_raw_info(file_name):
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


def instrument_name_correction(instrument_name):
    return "SANS2D" if instrument_name == "SAN" else instrument_name


def get_instrument_name_for_raw(file_name):
    instrument_name = get_from_raw_header(file_name, 0)
    return instrument_name_correction(instrument_name)


def get_run_number_for_raw(file_name):
    return int(get_from_raw_header(file_name, 1))


def get_number_of_periods_for_raw(file_name):
    return get_number_of_periods(get_raw_info, file_name)


def get_date_for_raw(file_name):
    def get_month(month_string):
        month_conversion = {"JAN": "01", "FEB": "02", "MAR": "03", "APR": "04",
                            "MAY": "05", "JUN": "06", "JUL": "07", "AUG": "08",
                            "SEP": "09", "OCT": "10", "NOV": "11", "DEC": "12"}
        month_upper = month_string.upper()
        if month_upper in month_conversion:
            return month_conversion[month_upper]
        else:
            raise RuntimeError("Cannot get measurement time. Invalid month in Raw file: " + month_upper)

    def get_raw_measurement_time(date_input, time_input):
        year = date_input[7:(7 + 4)]
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

    time_id = "r_endtime"
    date_id = "r_enddate"

    time = run_parameters.column(keys.index(time_id))
    date = run_parameters.column(keys.index(date_id))
    time = time[0]
    date = date[0]
    return get_raw_measurement_time(date, time)


# -----------------------------------------------
# SANS file Information
# -----------------------------------------------
class SANSFileInformation(with_metaclass(ABCMeta, object)):
    def __init__(self, file_name):
        self._file_name = file_name

    @abstractmethod
    def get_file_name(self):
        pass

    @abstractmethod
    def get_instrument(self):
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
    def get_run_number(self):
        pass

    @staticmethod
    def get_full_file_name(file_name):
        return find_sans_file(file_name)


class SANSFileInformationISISNexus(SANSFileInformation):
    def __init__(self, file_name):
        super(SANSFileInformationISISNexus, self).__init__(file_name)
        # Setup instrument name
        self._full_file_name = SANSFileInformation.get_full_file_name(self._file_name)
        instrument_name = get_instrument_name_for_isis_nexus(self._full_file_name)
        self._instrument = SANSInstrument.from_string(instrument_name)

        # Setup date
        self._date = get_date_for_isis_nexus(self._full_file_name)

        # Setup number of periods
        self._number_of_periods = get_number_of_periods_for_isis_nexus(self._full_file_name)

        # Setup run number
        self._run_number = get_run_number_for_isis_nexus(self._full_file_name)

        # Setup event mode check
        self._is_event_mode = get_event_mode_information(self._full_file_name)

    def get_file_name(self):
        return self._full_file_name

    def get_instrument(self):
        return self._instrument

    def get_date(self):
        return self._date

    def get_number_of_periods(self):
        return self._number_of_periods

    def get_run_number(self):
        return self._run_number

    def get_type(self):
        return FileType.ISISNexus

    def is_event_mode(self):
        return self._is_event_mode


class SANSFileInformationRaw(SANSFileInformation):
    def __init__(self, file_name):
        super(SANSFileInformationRaw, self).__init__(file_name)
        # Setup instrument name
        self._full_file_name = SANSFileInformation.get_full_file_name(self._file_name)
        instrument_name = get_instrument_name_for_raw(self._full_file_name)
        self._instrument = SANSInstrument.from_string(instrument_name)

        # Setup date
        self._date = get_date_for_raw(self._full_file_name)

        # Setup number of periods
        self._number_of_periods = get_number_of_periods_for_raw(self._full_file_name)

        # Setup run number
        self._run_number = get_run_number_for_raw(self._full_file_name)

    def get_file_name(self):
        return self._full_file_name

    def get_instrument(self):
        return self._instrument

    def get_date(self):
        return self._date

    def get_number_of_periods(self):
        return self._number_of_periods

    def get_run_number(self):
        return self._run_number

    def get_type(self):
        return FileType.ISISRaw


class SANSFileInformationFactory(object):
    def __init__(self):
        super(SANSFileInformationFactory, self).__init__()

    def create_sans_file_information(self, file_name):
        full_file_name = find_sans_file(file_name)
        if is_isis_nexus_single_period(full_file_name) or is_isis_nexus_multi_period(full_file_name):
            file_information = SANSFileInformationISISNexus(full_file_name)
        elif is_raw_single_period(full_file_name) or is_raw_multi_period(full_file_name):
            file_information = SANSFileInformationRaw(full_file_name)
        # TODO: ADD added nexus files here
        else:
            raise NotImplementedError("The file type you have provided is not implemented yet.")
        return file_information
