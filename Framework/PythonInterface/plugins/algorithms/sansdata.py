from copy import deepcopy
from dataclasses import dataclass, field
from datetime import datetime
import numpy as np


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

    def process_data(self):
        tmp_dict = {}
        for value in self.info:
            tmp = value.split('=')
            try:
                tmp_dict[tmp[0]] = tmp[1]
            except IndexError:
                pass
        self.info = tmp_dict
        self.assign_values()

    def assign_values(self):
        for att in self.__dict__.keys():
            self.assign_value(att)

    def assign_value(self, att, unique_name=None):
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


@dataclass
class FileSANS(DtClsSANS):
    section_name: str = 'File'
    _date_format = '%m/%d/%Y %I:%M:%S %p'

    def run_start(self):
        return np.datetime64(datetime.strptime(self.info['FromDate'] + ' ' + self.info['FromTime'],
                                               self._date_format))

    def run_end(self):
        return np.datetime64(datetime.strptime(self.info['ToDate'] + ' ' + self.info['ToTime'],
                                               self._date_format))


@dataclass
class SampleSANS(DtClsSANS):
    section_name: str = 'Sample'


@dataclass
class SetupSANS(DtClsSANS):
    section_name: str = 'Setup'


@dataclass
class CounterSANS(DtClsSANS):
    section_name: str = 'Counter'


@dataclass
class HistorySANS(DtClsSANS):
    section_name: str = 'History'


@dataclass
class CommentSANS(DtClsSANS):
    section_name: str = 'Comment'

    det1_x_value: float = 0.0
    det1_z_value: float = 0.0
    wavelength: float = 0.0

    st1_x_value: float = 0.0
    st1_x_offset: float = 0.0

    st1_y_value: float = 0.0
    st1_y_offset: float = 0.0

    st1_z_value: float = 0.0
    st1_z_offset: float = 0.0

    det1_omg_value: float = 0

    def assign_values(self):
        """
        one of the methods to add variable with unique name
        """
        super().assign_values()
        self.assign_value('selector_lambda_value', 'wavelength')


@dataclass
class CountsSANS(DtClsSANS):
    section_name: str = 'Counts'
    data: list = field(default_factory=list)

    def process_data(self):
        for line in self.info:
            if len(line) > 3:   # check if line has no zero length (' ', '\n', ...)
                tmp_data = line.split(',')
                self.data.append([float(i) for i in tmp_data])


class SANSdata(object):
    """
    This class describes the SANS-1_MLZ data structure
    will be used for SANS-1 data read-in and write-out routines
    """
    def __init__(self):
        self.file: FileSANS = FileSANS()
        self.sample: SampleSANS = SampleSANS()
        self.setup: SetupSANS = SetupSANS()
        self.counter: CounterSANS = CounterSANS()
        self.history: HistorySANS = HistorySANS()
        self.comment: CommentSANS = CommentSANS()
        self.counts: CountsSANS = CountsSANS()

        self._subsequence = [self.file, self.sample, self.setup, self.counter, self.history, self.comment, self.counts]

    def get_subsequence(self):
        return self._subsequence

    def analyze_source(self, filename):
        """
        read the SANS-1 raw file into the SANS-1 data object
        """
        with open(filename, 'r') as fhandler:
            unprocessed = fhandler.read()
        try:
            self.sort_data(unprocessed.split('%'))
        except IndexError as e:
            raise FileNotFoundError(e)

    @staticmethod
    def find_first_section_position(unprocessed):
        if unprocessed[0][0] != '\n':
            return 0
        return 1

    def sort_data(self, unprocessed):
        """
        initialize information for every section
        """
        pos = self.find_first_section_position(unprocessed)

        if len(unprocessed[pos:]) != len(self._subsequence):
            raise IndexError(f"Incorrect amount of sections: "
                             f"{len(unprocessed[pos:])} != {len(self._subsequence)}")

        for i in range(len(self._subsequence)):
            tmp = unprocessed[i + pos].split('\n')
            if tmp[0] != self._subsequence[i].section_name:
                raise IndexError(f"Section name doesn't match with expected: "
                                 f"'{tmp[0]}' != '{self._subsequence[i].section_name}'")

            self._subsequence[i].info = deepcopy(tmp[1:])
            self._subsequence[i].process_data()
