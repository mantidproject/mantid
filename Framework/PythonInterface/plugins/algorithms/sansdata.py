import pprint
from dataclasses import dataclass, asdict, field
from copy import deepcopy


@dataclass
class DtClsSANS:
    section_name: str = ''
    all_data: dict = field(default_factory=list)

    def split_data(self):
        tmp_dict = {}
        for value in self.all_data:
            tmp = value.split('=')
            try:
                tmp_dict[tmp[0]] = tmp[1]
            except IndexError:
                pass
        self.all_data = tmp_dict

        for att in self.__dict__.keys():
            try:
                try:
                    setattr(self, att, float(self.all_data[att]))
                except ValueError:
                    setattr(self, att, self.all_data[att])
            except KeyError:
                pass


@dataclass
class FileSANS(DtClsSANS):
    section_name: str = 'File'


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
    selector_lambda_value: float = 0

    st1_x_value: float = 0.0
    st1_x_offset: float = 0.0

    st1_y_value: float = 0.0
    st1_y_offset: float = 0.0

    st1_z_value: float = 0.0
    st1_z_offset: float = 0.0

    selector_ng_status: str = ''


@dataclass
class CountsSANS(DtClsSANS):
    section_name: str = 'Counts'
    data: list = field(default_factory=list)

    def split_data(self):
        for line in self.all_data:
            if len(line) > 3:  # TODO change
                tmp_data = line.split(',')
                self.data.append([float(i) for i in tmp_data])


class SANSdata(object):
    def __init__(self):
        self.file: FileSANS = FileSANS()
        self.sample: SampleSANS = SampleSANS()
        self.setup: SetupSANS = SetupSANS()
        self.counter: CounterSANS = CounterSANS()
        self.history: HistorySANS = HistorySANS()
        self.comment: CommentSANS = CommentSANS()
        self.counts: CountsSANS = CountsSANS()

        self._subsequence = [self.file, self.sample, self.setup, self.counter, self.history, self.comment, self.counts]

    def analyze_source(self, filename):
        with open(filename, 'r') as fhandler:
            unparsed = fhandler.read()
        self.sort_data(unparsed.split('%'))

    @staticmethod
    def find_first_data():
        return 1

    def sort_data(self, unparsed):
        f = self.find_first_data()
        for i in range(len(self._subsequence)):
            tmp = unparsed[i + f].split('\n')
            if tmp[0] == self._subsequence[i].section_name:
                self._subsequence[i].all_data = deepcopy(tmp[1:])
                self._subsequence[i].split_data()


def main():
    data = SANSdata()
    data.analyze_source("/home/andrii/repositories/AndriiDemk/mantid/"
                        "build/ExternalData/Testing/Data/UnitTest/D0511339.001")
    # pprint.pprint(asdict(data.comment))
    pprint.pp(asdict(data.file))


if __name__ == "__main__":
    main()
