# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
"""
Class which loads and stores a single DNS datafile in a dictionary
"""

import os

from mantidqtinterfaces.dns.data_structures.object_dict import ObjectDict
from mantidqtinterfaces.dns.helpers.list_range_converters import \
    list_to_multirange as list_to_range


# copied from dns dataset
def get_proposal_from_filname(filename, filenumber):
    return filename.replace(f'_{filenumber:06d}.d_dat', '')


def get_bank_positions(sampledata, rounding_limit=0.05):
    new_arr = []
    inside = False
    banks = set(entry['det_rot'] for entry in sampledata)
    for bank in banks:
        for compare in new_arr:
            if abs(compare - bank) < rounding_limit:
                inside = True
                break
            inside = False
        if not inside:
            new_arr.append(bank)
    return new_arr


def _get_max_key_length(dataset):
    return max([len(a) for a in dataset.keys()]) + 6 + 4


def _get_field_string(fields, llens):
    field_items = sorted(fields.items())
    spacer = " " * (llens + 1)
    stringlist = [(f"{spacer}{key:4.2f}: {value}")
                  for key, value in field_items]
    return ",\n".join(stringlist)


def _get_path_string(dataset, samplename):
    path = dataset[samplename].pop('path')
    return f"'path': '{path}',\n"


def _get_sample_string(samplename, llens):
    return f"'{samplename:s}': {{".rjust(llens)


def _get_datapath(entry, path):
    proposal = get_proposal_from_filname(entry['filename'],
                                         entry['filenumber'])
    return os.path.join(path, proposal)


def _convert_list_to_range(dataset):
    for sample_typ, det_rots in dataset.items():
        for det_rot, filenumbers in det_rots.items():
            if det_rot != 'path':
                dataset[sample_typ][det_rot] = list_to_range(filenumbers)
    return dataset


def _get_closest_bank_key(banks, det_rot):
    rounding_limit = 0.05
    compare = det_rot
    for compare in [x for x in banks.keys() if x != 'path']:
        if abs(compare - det_rot) < rounding_limit:
            break
        compare = det_rot
    return compare


def _create_new_datatype(dataset, datatype, det_rot, entry, path):
    dataset[datatype] = {
        det_rot: [entry['filenumber']],
        'path': _get_datapath(entry, path)
    }


def _add_or_create_filelist(dataset, datatype, det_rot, entry):
    compare = _get_closest_bank_key(dataset[datatype], det_rot)
    fnlist = dataset[datatype].get(compare, None)
    if fnlist is not None:
        fnlist.append(entry['filenumber'])
    else:
        dataset[datatype][compare] = [entry['filenumber']]


class DNSTofDataset(ObjectDict):
    """
    class for storing data of a multiple dns datafiles
    this is a dictionary  but can also be accessed like atributes
    """

    def __init__(self, data, path, issample=True):
        super().__init__()
        self['issample'] = issample
        self['banks'] = get_bank_positions(data)
        self['datadic'] = self.create_dataset(data, path)

    def format_dataset(self):
        """Formating the dictionary to a nicely indented string"""

        dataset = self.datadic
        llens = _get_max_key_length(dataset)
        dataset_string = '{\n'
        for samplename, fields in dataset.items():
            dataset_string += _get_sample_string(samplename, llens)
            dataset_string += _get_path_string(dataset, samplename)
            dataset_string += _get_field_string(fields, llens)
            dataset_string += "},\n"
        dataset_string += "}"
        return dataset_string

    def _get_nb_banks(self, sampletype=None):
        dic = self.datadic.get(sampletype, 0)
        if dic:
            return len(dic.keys()) - 1
        return 0

    def get_vana_filename(self):
        vanafilename = [x for x in self.datadic.keys() if '_vana' in x]
        if len(vanafilename) == 0:
            vanafilename = ''
        elif len(vanafilename) > 1:
            vanafilename = ''
        else:
            vanafilename = vanafilename[0]
        return vanafilename

    def get_empty_filename(self):
        emptyfilename = [
            x for x in self.datadic.keys() if ('_empty' in x or '_leer' in x)
        ]
        if len(emptyfilename) == 0:
            emptyfilename = ''
        elif len(emptyfilename) > 1:
            emptyfilename = ''
        else:
            emptyfilename = emptyfilename[0]
        return emptyfilename

    def get_sample_filename(self):
        samplefilename = list(self.datadic.keys())
        # if len(samplefilename) > 1:
        #    pass
        return samplefilename[0]

    def get_nb_sample_banks(self):
        return self._get_nb_banks(self.get_sample_filename())

    def get_nb_vana_banks(self):
        return self._get_nb_banks(self.get_vana_filename())

    def get_nb_empty_banks(self):
        return self._get_nb_banks(self.get_empty_filename())

    @staticmethod
    def create_dataset(data, path):
        """ creates a smaller dictionary used in the reduction script
            of the form
            dict[datatype][path/det_rot] = list(filenumbers)
        """
        dataset = {}
        for entry in data:
            datatype = entry['samplename']
            det_rot = entry['det_rot']
            if datatype in dataset:
                _add_or_create_filelist(dataset, datatype, det_rot, entry)
            else:
                _create_new_datatype(dataset, datatype, det_rot, entry, path)
        dataset = _convert_list_to_range(dataset)
        return dataset
