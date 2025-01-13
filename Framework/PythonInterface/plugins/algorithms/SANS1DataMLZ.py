from datetime import datetime
import numpy as np
from scipy import ndimage
import re
import itertools
from copy import deepcopy


class DtClsSANS:
    """
    Parent class for all subsequent classes.
    All subsequent classes are helper classes to describe
    each section of a SANS-1 datafile ('File', 'Sample', etc.);
    """

    section_name: str = ""

    def __init__(self):
        self.info = {}

    def process_data(self, unprocessed):
        """
        process data to better access
        data saved to dictionary 'info' and
        can be obtained by info['name']
        """
        unprocessed = self._data_preparation(unprocessed)
        tmp_dict = {}
        for value in unprocessed:
            tmp = value.split("=")
            try:
                tmp_dict[tmp[0]] = tmp[1]
            except IndexError:
                pass
        self.info = tmp_dict
        self._assign_values()

    def get_values_dict(self):
        """
        :return: dictionary with the most relevant variables of helper class
        """
        values = deepcopy(self.__dict__)
        if "info" in values.keys():
            del values["info"]
        if "data" in values.keys():
            del values["data"]
        return values

    def _assign_values(self):
        for att in self.get_values_dict().keys():
            self._assign_value(att)

    def _assign_value(self, att: str, unique_name: str = None):
        """
        assign to class parameter
        corresponding info['att']
        """
        try:
            if unique_name:
                setattr(self, unique_name, float(self.info[att]))
            else:
                setattr(self, att, float(self.info[att]))
        except ValueError:
            if unique_name:
                setattr(self, unique_name, self.info[att])
            else:
                setattr(self, att, self.info[att])
        except KeyError:
            pass

    @staticmethod
    def _data_preparation(data):
        return data.split("\n")


class FileSANS(DtClsSANS):
    section_name: str = "File"
    pattern = re.compile(r"(%File\n)([^%]*)")
    _date_format = "%m/%d/%Y %I:%M:%S %p"

    def run_start(self):
        return np.datetime64(datetime.strptime(self.info["FromDate"] + " " + self.info["FromTime"], self._date_format))

    def run_end(self):
        return np.datetime64(datetime.strptime(self.info["ToDate"] + " " + self.info["ToTime"], self._date_format))

    def get_title(self):
        file_name = self.info["FileName"].split(".")
        return f"{file_name[0]}/{int(file_name[1])}"


class SampleSANS(DtClsSANS):
    section_name: str = "Sample"
    pattern = re.compile(r"(%Sample\n)([^%]*)")

    def __init__(self):
        super().__init__()
        self.position: int = 0
        self.thickness: float = 0

    def _assign_values(self):
        self._assign_value("Position", "position")
        self._assign_value("Thickness", "thickness")


class SetupSANS(DtClsSANS):
    section_name: str = "Setup"
    pattern = re.compile(r"(%Setup\n)([^%]*)")

    def __init__(self):
        super().__init__()
        self.wavelength: float = 0.0
        self.wavelength_error_mult: float = 0.1  # wavelength spread up to 10%
        self.l2: float = 20.0
        self.collimation: float = 0.0

    def _assign_values(self):
        self._assign_value("Lambda", "wavelength")
        self._assign_value("SD", "l2")
        self._assign_value("Collimation", "collimation")


class CounterSANS(DtClsSANS):
    section_name: str = "Counter"
    pattern = re.compile(r"(%Counter\n)([^%]*)")

    def __init__(self):
        super().__init__()
        self.sum_all_counts: float = 0
        self.duration: float = 0
        self.monitor1: float or None = None
        self.monitor2: float or None = None

    def get_monitors(self):
        if self.monitor1 is None and self.monitor2 is None:
            return []
        return [self.monitor1, self.monitor2]

    def process_data(self, unprocessed):
        super().process_data(unprocessed)
        # after _assign_value() monitor can be assigned to '' ->
        if isinstance(self.monitor1, str):
            self.monitor1 = None
        if isinstance(self.monitor2, str):
            self.monitor2 = None

    def _assign_values(self):
        self._assign_value("Sum", "sum_all_counts")
        self._assign_value("Time", "duration")
        self._assign_value("Moni1", "monitor1")
        self._assign_value("Moni2", "monitor2")


class HistorySANS(DtClsSANS):
    section_name: str = "History"
    pattern = re.compile(r"(%History\n)([^%]*)")

    def __init__(self):
        super().__init__()
        self.transmission: float = 0.0
        self.scaling: float = 0.0
        self.probability: float = 0.0
        self.aperture: float = 0.0

    def _assign_values(self):
        self._assign_value("Transmission", "transmission")
        self._assign_value("Scaling", "scaling")
        self._assign_value("Probability", "probability")
        self._assign_value("Aperture", "aperture")


class CommentSANS(DtClsSANS):
    section_name: str = "Comment"
    pattern = re.compile(r"(%Comment.*\n)([^%]*)")

    @staticmethod
    def _data_preparation(data):
        return data


class CountsSANS(DtClsSANS):
    section_name: str = "Counts"
    pattern = re.compile(r"(%Counts\n)([^%]*)")

    def __init__(self):
        super().__init__()
        self.data = np.ndarray(shape=(128, 128), dtype=float)

    def process_data(self, unprocessed):
        pattern = re.compile(r"\d+")
        matches = pattern.findall(unprocessed)
        self.data = np.array([count for count in matches], dtype=float).reshape(128, 128)


class SANSdata:
    """
    This class describes the SANS-1_MLZ data structure and
    will be used for SANS-1 data read-in and write-out routines.
    Data from each section of a raw datafile are assigned to a
    class that is named correspondingly.
    """

    def __init__(self):
        self.file: FileSANS = FileSANS()
        self.sample: SampleSANS = SampleSANS()
        self.setup: SetupSANS = SetupSANS()
        self.counter: CounterSANS = CounterSANS()
        self.history: HistorySANS = HistorySANS()
        self.comment: CommentSANS = CommentSANS()
        self.counts: CountsSANS = CountsSANS()

        # main sections that should be in the datafile
        self._subsequence = [self.file, self.sample, self.setup, self.counter, self.history, self.counts]

        self.logs: dict = {"notice": [], "warning": []}

    def get_subsequence(self) -> list:
        return self._subsequence

    def spectrum_amount(self) -> int:
        n_rows = self.file.info["DataSizeY"]
        n_bins = self.file.info["DataSizeX"]
        n_spec = n_rows * n_bins
        n_spec += len(self.counter.get_monitors())
        return n_spec

    def data_y(self) -> np.array:
        """
        :return: 1 dimensional counts data array for mantid workspace
        """
        data_y = self.counts.data.reshape(-1)
        data_y = np.append(data_y, self.counter.get_monitors())
        return data_y

    def data_x(self, wavelength=None) -> np.ndarray:
        # ToDo warning! To be fixed.
        # if you have more than 2 columns (time-of-flight data) then
        # lines below won't work properly
        # Better to introduce a parameter n_columns and
        # use data_x = np.zeros(n_columns * n_spec), etc
        # To be resolved when the TOF mode has been implemented.
        if wavelength is None:
            wavelength = self.setup.wavelength
        data_x = np.zeros(2 * self.spectrum_amount())
        data_x.fill(wavelength * (1 + self.setup.wavelength_error_mult))
        data_x[::2] -= wavelength * (self.setup.wavelength_error_mult * 2)
        return data_x

    def data_e(self) -> np.ndarray:
        data_e = np.array(np.sqrt(self.counts.data.reshape(-1)))
        data_e = np.append(data_e, self.counter.get_monitors())
        return data_e

    def beamcenter_x_y(self) -> tuple:
        beamcenter_y, beamcenter_x = ndimage.center_of_mass(self.counts.data)
        return beamcenter_x, beamcenter_y

    def analyze_source(self, filename: str, comment: bool = False):
        """
        read SANS-1 .001 raw files into the SANS-1 data object
        """
        with open(filename, "r") as file_handler:
            unprocessed = file_handler.read()
        self._initialize_info(unprocessed)
        if comment:
            self._find_comments(unprocessed)
        self._check_data()

    def _initialize_info(self, unprocessed):
        """
        search for main sections; if a section doesn't exist -> raise Error
        """
        for section in self._subsequence:
            matches = section.pattern.finditer(unprocessed)
            match = next(matches, False)
            if not match:
                raise RuntimeError(f"Failed to find '{section.section_name}' section")
            section.process_data(match.groups()[1])

    def _find_comments(self, unprocessed):
        """
        search for comment sections
        """
        matches = self.comment.pattern.finditer(unprocessed)
        tmp = [match.groups()[1].split("\n") for match in matches]
        if len(tmp) != 0:
            self.comment.process_data(list(itertools.chain(*tmp)))
        else:
            self.logs["warning"].append("Failed to find 'Comment *' sections.")

    def _check_data_size(self):
        for param in ("DataSizeY", "DataSizeX"):
            try:
                self.file.info[param] = int(self.file.info[param])
            except KeyError:
                self.file.info[param] = 128
                self.logs["notice"].append(f"{param} is not specified in the datafile. {param} set to 128.")
            except ValueError:
                self.file.info[param] = 128
                self.logs["notice"].append(f"{param} is not specified in the datafile. {param} set to 128.")

    def _check_data(self):
        """
        logging if there were any problems
        during the data processing
        """
        self._check_data_size()
        if isinstance(self.setup.collimation, str) or self.setup.collimation == 0.0:
            self.logs["warning"].append("Collimation is not specified in the datafile.")

        if isinstance(self.setup.l2, str) or self.setup.l2 == 0.0:
            self.logs["warning"].append("SD - sample detector distance ('l2') is not specified in the datafile.")

        if isinstance(self.setup.wavelength, str) or self.setup.wavelength == 0.0:
            self.logs["warning"].append("Lambda (wavelength) is not specified in the datafile. Wavelength is set to user's input.")

        if isinstance(self.counter.sum_all_counts, str) or self.counter.sum_all_counts == 0.0:
            self.logs["warning"].append("Sum of all counts is not specified in the datafile.")

        if isinstance(self.counter.duration, str) or self.counter.duration == 0.0:
            self.logs["warning"].append("Duration of the measurement is not specified in the datafile.")

        if (self.counter.monitor2 is None) or (self.counter.monitor2 == 0.0):
            self.logs["warning"].append("Monitor2 is not specified in the datafile. Monitor2 is set to 'None'.")

        if (self.counter.monitor1 is None) or (self.counter.monitor1 == 0.0):
            self.logs["notice"].append("Monitor1 is not specified in the datafile. Monitor1 is set to 'None'.")
