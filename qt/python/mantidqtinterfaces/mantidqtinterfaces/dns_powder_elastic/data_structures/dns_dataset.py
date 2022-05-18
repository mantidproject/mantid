# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +

"""
Class which loads and stores a single DNS datafile in a dictionary.
"""

import os

from mantidqtinterfaces.dns_powder_elastic.data_structures.dns_binning import \
    DNSBinning
from mantidqtinterfaces.dns_powder_elastic.data_structures. \
    dns_standard_interpolator import \
    interpolate_standard
from mantidqtinterfaces.dns_powder_elastic.data_structures.field_names import \
    field_dict
from mantidqtinterfaces.dns_powder_tof.data_structures.object_dict import \
    ObjectDict
from mantidqtinterfaces.dns_powder_tof.helpers.list_range_converters import \
    list_to_multirange


class DNSDataset(ObjectDict):
    """
    Class for storing data of a multiple dns datafiles
    this is a dictionary  but can also be accessed like attributes.
    """

    def __init__(self, data, path, is_sample=True, fields=None):
        super().__init__()
        self.is_sample = is_sample
        if not is_sample and fields is not None:
            data = remove_non_measured_fields(data, fields)
        self.script_name = create_script_name(data)
        self.banks = get_bank_positions(data)
        self.fields = get_sample_fields(data)
        self.ttheta = automatic_ttheta_binning(data)
        self.omega = automatic_omega_binning(data)
        self.data_dic = create_dataset(data, path)

    def format_dataset(self):
        """
        Formatting the dictionary to a nicely indented string.
        """
        for fields in self.data_dic.values():
            for field, file_numbers in fields.items():
                if field != 'path':
                    fields[field] = list_to_multirange(file_numbers)

        l_lens = max([len(a) for a in self.data_dic]) + 6 + 4
        dataset_string = '{\n'
        for sample_name, fields in self.data_dic.items():
            lmax = max([len(key) for key in fields] + [0])

            dataset_string += f"'{sample_name:s}' : {{".rjust(l_lens)

            dataset_string += f"{' ' * (lmax - 4)}'path' : " \
                              f"'{fields.pop('path')}" \
                              f"',\n"
            dataset_string += ",\n".join([
                f"{' ' * (lmax - len(key) + l_lens)}'{key:s}' : {value}"
                for key, value in sorted(fields.items())
            ])
            dataset_string += "},\n"
        dataset_string += "}"
        return dataset_string

    def create_subtract(self):
        subtract = []
        for sample, workspace_list in self.data_dic.items():
            for workspace in workspace_list:
                if workspace != 'path':
                    subtract.append(f"{sample}_{workspace}")
        return subtract

    def interpolate_standard(self, banks, script_name, parent):
        self.data_dic, n_error = interpolate_standard(self.data_dic, banks,
                                                      script_name)
        if n_error:
            parent.raise_error(
                'Error: Interpolation of standard data with '
                'different counting times for the same sample '
                'type and field is not supported.',
                critical=True)

    def get_nb_banks(self, sample_type=None):
        """
        Returns dict with sample names and number of banks, can be
        0 or the same for all samples.
        """
        if self.data_dic.get(sample_type, 0):
            return len(self.banks)
        return 0


# helper functions
def create_script_name(sample_data):
    lowest_fn = min([entry['file_number'] for entry in sample_data])
    highest_fn = max([entry['file_number'] for entry in sample_data])
    sample_name = '_and_'.join(
        set(entry['sample_type'] for entry in sample_data))
    return f'{sample_name}_{lowest_fn}_to_{highest_fn}.py'


def automatic_ttheta_binning(sample_data):
    det_rot = [-x['det_rot'] for x in sample_data]
    ttheta_max = (max(det_rot) + 115)
    ttheta_min = (min(det_rot))
    ttheta = list(set(det_rot))
    add_ttheta = []
    for tt in ttheta:
        add_ttheta.append(tt + 5)
    ttheta.extend(add_ttheta)
    ttheta_step = get_ttheta_step(ttheta)
    return DNSBinning(ttheta_min, ttheta_max, ttheta_step)


def get_ttheta_step(angles, rounding_limit=0.05):
    angles = sorted(angles)
    diff_rot = [abs(angles[i] - angles[i + 1]) for i in range(len(angles) - 1)]
    diff_rot = [i for i in diff_rot if i >= rounding_limit]
    diff_rot = list_to_set(diff_rot)
    if len(diff_rot) > 1:
        diff_diff_rot = [
            abs(diff_rot[i] - diff_rot[i + 1])
            for i in range(len(diff_rot) - 1)
        ]
        diff_rot.extend(diff_diff_rot)
    step = round_step(min(diff_rot))
    return step


def round_step(step, rounding_limit=0.05):
    likely_steps = [
        1, 2, 5 / 2, 5 / 3, 5 / 4, 5 / 6, 5 / 7, 5 / 8, 5 / 9, 5 / 10, 5 / 15,
        5 / 20
    ]
    for i in likely_steps:
        if abs(step - i) <= rounding_limit:
            return i
    return step


def get_omega_step(angles, rounding_limit=0.05):
    angles = sorted(angles)
    diff_rot = [abs(angles[i] - angles[i + 1]) for i in range(len(angles) - 1)]
    diff_rot = [i for i in diff_rot if i >= rounding_limit]
    if not diff_rot:
        return 1
    diff_rot = list_to_set(diff_rot)
    min_step = min(diff_rot)
    return min_step


def list_to_set(bank_list, rounding_limit=0.05):
    '''
    Finds set of bank positions not further apart than rounding_limit
    group starts with the smallest bank position,
    which is also the returned value, average would not be unique.
    '''
    sorted_banks = sorted(bank_list)
    if not sorted_banks:
        return []
    max_bank = [sorted_banks[0]]
    for bank in sorted_banks[1:]:
        if abs(max_bank[-1] - bank) > rounding_limit:
            max_bank.append(bank)
    return max_bank


def automatic_omega_binning(sample_data):
    omega = [x['sample_rot'] - x['det_rot'] for x in sample_data]
    omega_max = max(omega)
    omega_min = min(omega)
    sample_rot = [x['sample_rot'] for x in sample_data]
    omega_step = get_omega_step(sample_rot)
    return DNSBinning(omega_min, omega_max, omega_step)


def get_proposal_from_filename(filename, file_number):
    return filename.replace(f'_{file_number:06d}.d_dat', '')


def get_sample_fields(sample_data):
    return set(entry['field'] for entry in sample_data)


def create_dataset(data, path):
    """
    Converting data from fileselector to a smaller dictionary.
    """
    dataset = {}
    for entry in data:
        datatype = get_datatype_from_sample_name(entry['sample_name'])
        field = field_dict.get(entry['field'], entry['field'])
        proposal = get_proposal_from_filename(entry['filename'],
                                              entry['file_number'])
        data_path = os.path.join(path, proposal)
        if datatype in dataset:
            if field in dataset[datatype].keys():
                dataset[datatype][field].append(entry['file_number'])
            else:
                dataset[datatype][field] = [entry['file_number']]
        else:
            dataset[datatype] = {}
            dataset[datatype][field] = [entry['file_number']]
            dataset[datatype]['path'] = data_path
    return dataset


def get_datatype_from_sample_name(sample_name):
    datatype = sample_name.strip('_')
    datatype = datatype.replace(' ', '')
    if sample_name == 'leer':  # compatibility with old names
        datatype = 'empty'
    return datatype


def remove_non_measured_fields(standard_data, sample_fields):
    standard_data = [
        entry for entry in standard_data if entry['field'] in sample_fields
    ]
    return standard_data


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
