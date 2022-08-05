from dataclasses import dataclass, field, asdict
from datetime import datetime
import numpy as np
import re
import itertools


@dataclass
class DtClsSANS:
    """
    Parent dataclass for all next dataclasses.
    All next dataclasses are helper classes to describe
    each section of SANS-1 datafile ('File', 'Sample',
    'Setup', 'Counter', 'History', 'Comment', 'Counts');
    You can simply add variables to helper dataclass:
     1. variable must have identical name as in raw data file;
     2. variable must have annotation and initial value;
     example -> new_var: float = 0
    """
    section_name: str = ''
    info: dict = field(default_factory=list)

    def process_data(self, unprocessed):
        unprocessed = self._data_preparation(unprocessed)
        tmp_dict = {}
        for value in unprocessed:
            tmp = value.split('=')
            try:
                tmp_dict[tmp[0]] = tmp[1]
            except IndexError:
                pass
        self.info = tmp_dict
        self._assign_values()

    def get_values_dict(self):
        """
        return dictionary with variables of helper class
        (without 'section_name' and 'info')
        """
        values = asdict(self)
        del values['section_name']
        del values['info']
        return values

    def _assign_values(self):
        for att in self.get_values_dict().keys():
            self._assign_value(att)

    def _assign_value(self, att, unique_name=None):
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
        return data.split('\n')


@dataclass
class FileSANS(DtClsSANS):
    section_name: str = 'File'
    pattern = re.compile(r'(%File\n)([^%]*)')
    type = '001'
    _date_format = '%m/%d/%Y %I:%M:%S %p'

    def run_start(self):
        return np.datetime64(datetime.strptime(self.info['FromDate'] + ' ' + self.info['FromTime'],
                                               self._date_format))

    def run_end(self):
        return np.datetime64(datetime.strptime(self.info['ToDate'] + ' ' + self.info['ToTime'],
                                               self._date_format))

    def get_title(self):
        file_name = self.info['FileName'].split('.')
        return f"{file_name[0]}/{int(file_name[1])}"

    def process_data(self, unprocessed):
        super().process_data(unprocessed)
        self.type = self.info['FileName'].split('.')[1]


@dataclass
class SampleSANS(DtClsSANS):
    section_name: str = 'Sample'
    pattern = re.compile(r'(%Sample\n)([^%]*)')
    position: int = 0
    thickness: float = 0

    def _assign_values(self):
        """
        one of the methods to add variable with unique name
        """
        super()._assign_values()
        self._assign_value('Position', 'position')
        self._assign_value('Thickness', 'thickness')


@dataclass
class SetupSANS(DtClsSANS):
    section_name: str = 'Setup'
    pattern = re.compile(r'(%Setup\n)([^%]*)')
    wavelength: int = 0
    sample_detector_distance: float = 0

    def _assign_values(self):
        """
        one of the methods to add variable with unique name
        """
        super()._assign_values()
        self._assign_value('Lambda', 'wavelength')
        self._assign_value('SD', 'sample_detector_distance')


@dataclass
class CounterSANS(DtClsSANS):
    section_name: str = 'Counter'
    pattern = re.compile(r'(%Counter\n)([^%]*)')

    sum_all_counts: float = 0
    duration: float = 0
    monitor1: float = 0
    monitor2: float = 0

    def _assign_values(self):
        """
        one of the methods to add variable with unique name
        """
        super()._assign_values()
        self._assign_value('Sum', 'sum_all_counts')
        self._assign_value('Time', 'duration')
        self._assign_value('Moni1', 'monitor1')
        self._assign_value('Moni2', 'monitor2')

    def is_monitors_exist(self):
        if self.monitor1 != 0 or self.monitor2 != 0:
            return True
        return False


@dataclass
class HistorySANS(DtClsSANS):
    section_name: str = 'History'
    pattern = re.compile(r'(%History\n)([^%]*)')
    transmission: float = 0.0
    scaling: float = 0.0
    probability: float = 0.0
    beamcenter_x: float = 0.0
    beamcenter_y: float = 0.0
    aperture: float = 0.0

    def _assign_values(self):
        """
        one of the methods to add variable with unique name
        """
        super()._assign_values()
        self._assign_value('Transmission', 'transmission')
        self._assign_value('Scaling', 'scaling')
        self._assign_value('Probability', 'probability')
        self._assign_value('BeamcenterX', 'beamcenter_x')
        self._assign_value('BeamcenterY', 'beamcenter_y')
        self._assign_value('Aperture', 'aperture')


@dataclass
class CommentSANS(DtClsSANS):
    section_name: str = 'Comment'
    pattern = re.compile(r'(%Comment.*\n)([^%]*)')

    wavelength: float = 0.0

    def _assign_values(self):
        """
        one of the methods to add variable with unique name
        """
        super()._assign_values()
        self._assign_value('selector_lambda_value', 'wavelength')

    def set_wavelength(self, input_wavelength):
        if input_wavelength > 0:
            self.wavelength = input_wavelength
            self.info['selector_lambda_value'] = input_wavelength

    @staticmethod
    def _data_preparation(data):
        return data


@dataclass
class CountsSANS(DtClsSANS):
    section_name: str = 'Counts'
    pattern = re.compile(r'(%Counts\n)([^%]*)')
    data_type = '001'
    data: list = field(default_factory=list)

    def process_data(self, unprocessed):
        if self.data_type == '001':
            self._process_001(unprocessed)
        elif self.data_type == '002':
            self._process_002(unprocessed)

    def _process_001(self, unprocessed):
        pattern = re.compile(r'\d+')
        matches = pattern.findall(unprocessed)
        self.data = [float(count) for count in matches]

    def _process_002(self, unprocessed):
        pattern = re.compile(r'[-+]?\d+\.\d*e[-+]\d*')
        matches = pattern.findall(unprocessed)
        self.data = [float(count) for count in matches]


@dataclass
class ErrorsSANS(DtClsSANS):
    section_name: str = 'Errors'
    pattern = re.compile(r'(%Errors\n)([^%]*)')
    data: list = field(default_factory=list)

    def process_data(self, unprocessed):
        pattern = re.compile(r'[-+]?\d+\.\d*e[-+]\d*')
        matches = pattern.findall(unprocessed)
        self.data = [float(count) for count in matches]


class SANSdata(object):
    """
    This class describes the SANS-1_MLZ data structure
    will be used for SANS-1 data read-in and write-out routines.
    Data from each section of raw file contains in the variable
    with the same name.
    """

    def __init__(self):
        self.file: FileSANS = FileSANS()
        self.sample: SampleSANS = SampleSANS()
        self.setup: SetupSANS = SetupSANS()
        self.counter: CounterSANS = CounterSANS()
        self.history: HistorySANS = HistorySANS()
        self.comment: CommentSANS = CommentSANS()
        self.counts: CountsSANS = CountsSANS()
        self.errors: ErrorsSANS = ErrorsSANS()

        self._subsequence = [self.file, self.sample, self.setup, self.counter, self.history, self.counts]

    def get_subsequence(self):
        return self._subsequence

    def analyze_source(self, filename, comment=False):
        """
        read the SANS-1.001/002 raw files into the SANS-1 data object
        """
        with open(filename, 'r') as fhandler:
            unprocessed = fhandler.read()
        file_type = filename.split(".")[-1]
        if file_type == "001":
            self._initialize_info(unprocessed)
        elif file_type == "002":
            self.counts.data_type = '002'
            self._subsequence = [self.file, self.sample, self.setup, self.history, self.counts, self.errors]
            self._initialize_info(unprocessed)
        else:
            raise FileNotFoundError("Incorrect file")
        if comment:
            self._find_comments(unprocessed)

    def _initialize_info(self, unprocessed):
        for section in self._subsequence:
            matches = section.pattern.finditer(unprocessed)
            match = next(matches, False)
            if not match:
                raise FileNotFoundError(f"Failed to find '{section.section_name}' section")
            section.process_data(match.groups()[1])

    def _find_comments(self, unprocessed):
        matches = self.comment.pattern.finditer(unprocessed)
        tmp = [match.groups()[1].split('\n') for match in matches]
        self.comment.process_data(list(itertools.chain(*tmp)))
