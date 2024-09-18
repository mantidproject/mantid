# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2023 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +

"""
Class which loads and stores DNS powder elastic datafile information
in a dictionary.
"""

import os
import numpy as np

from mantidqtinterfaces.dns_powder_elastic.data_structures.dns_binning import DNSBinning
from mantidqtinterfaces.dns_powder_elastic.data_structures.field_names import field_dict
from mantidqtinterfaces.dns_powder_tof.data_structures.object_dict import ObjectDict
from mantidqtinterfaces.dns_powder_tof.helpers.list_range_converters import list_to_multirange


class DNSElasticDataset(ObjectDict):
    """
    Class for storing data of multiple DNS datafiles.
    This is a dictionary but can also be accessed like attributes.
    """

    def __init__(self, data, path, is_sample=True, banks=None, fields=None, ignore_van=False):
        super().__init__()
        if data:
            self["is_sample"] = is_sample
            if is_sample:
                self["angle_fields_data"] = get_sample_fields_each_bank(data)
            if not is_sample and banks is not None:
                data = remove_unnecessary_standard_banks(data, banks)
            if not is_sample and fields is not None:
                data = remove_unnecessary_standard_fields(data, fields, ignore_van)
            self["banks"] = get_bank_positions(data)
            self["fields"] = get_sample_fields(data)
            self["omega"] = automatic_omega_binning(data)
            self["data_dic"] = create_dataset(data, path)

    def format_dataset(self):
        """
        Formatting the dictionary to a nicely indented string.
        """
        dataset = self.data_dic
        for fields in dataset.values():
            for field, file_numbers in fields.items():
                if field != "path":
                    fields[field] = list_to_multirange(file_numbers)

        tab_indent = 4
        special_char_indent = 5
        dataset_string = "{\n"
        for sample_name, fields in dataset.items():
            dataset_string += "".rjust(tab_indent)
            dataset_string += _get_sample_string(sample_name)
            dataset_string += _get_path_string(dataset, sample_name)
            total_indent = tab_indent + special_char_indent + len(sample_name)
            dataset_string += _get_field_string(fields, total_indent)
            dataset_string += "},\n"
        dataset_string += "}"
        return dataset_string

    def create_subtract(self):
        subtract = []
        for sample, workspace_list in self.data_dic.items():
            for workspace in workspace_list:
                if workspace != "path":
                    subtract.append(f"{sample}_{workspace}")
        return subtract

    def get_number_banks(self, sample_type=None):
        """
        Returns dict with sample names and number of banks, can be
        0 or the same for all samples.
        """
        if self.data_dic.get(sample_type, 0):
            return len(self.banks)
        return 0


def round_step(step, rounding_limit=0.05):
    likely_steps = np.arange(0, 5, 0.1)
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
    """
    Finds set of bank positions not further apart than rounding_limit
    group starts with the smallest bank position,
    which is also the returned value, average would not be unique.
    """
    sorted_banks = sorted(bank_list)
    if not sorted_banks:
        return []
    max_bank = [sorted_banks[0]]
    for bank in sorted_banks[1:]:
        if abs(max_bank[-1] - bank) > rounding_limit:
            max_bank.append(bank)
    return max_bank


def automatic_omega_binning(sample_data):
    if sample_data:
        omega = [x["sample_rot"] - x["det_rot"] for x in sample_data]
        omega_max = max(omega)
        omega_min = min(omega)
        sample_rot = [x["sample_rot"] for x in sample_data]
        omega_step = get_omega_step(sample_rot)
        return DNSBinning(omega_min, omega_max, omega_step)


def get_proposal_from_filename(filename, file_number):
    return filename.replace(f"_{file_number:06d}.d_dat", "")


def get_sample_fields(sample_data):
    field_list = []
    for entry in sample_data:
        field = field_dict.get(entry["field"], entry["field"])
        field_list.append(field)
    field_set = set(field_list)
    return field_set


def create_dataset(data, path):
    """
    Converting data from file selector to a smaller dictionary.
    """
    dataset = {}
    for entry in data:
        datatype = get_datatype_from_sample_name(entry["sample_name"])
        field = field_dict.get(entry["field"], entry["field"])
        data_path = os.path.join(path, entry["filename"])
        file_number_string = str(entry["file_number"])
        file_number_string_length = len(file_number_string)
        star_pattern = "*" * file_number_string_length
        data_path_modified = data_path.replace(file_number_string, star_pattern)
        if datatype in dataset:
            if field in dataset[datatype].keys():
                dataset[datatype][field].append(entry["file_number"])
            else:
                dataset[datatype][field] = [entry["file_number"]]
        else:
            dataset[datatype] = {}
            dataset[datatype][field] = [entry["file_number"]]
            dataset[datatype]["path"] = data_path_modified
    return dataset


def get_datatype_from_sample_name(sample_name):
    datatype = sample_name.strip("_")
    datatype = datatype.replace(" ", "")
    if sample_name == "leer":  # compatibility with old names
        datatype = "empty"
    return datatype


def remove_unnecessary_standard_banks(standard_data, sample_banks, rounding_limit=0.05):
    standard_data_clean = []
    for file_dict in standard_data:
        for sample_bank in sample_banks:
            if np.isclose(file_dict["det_rot"], sample_bank, atol=rounding_limit):
                standard_data_clean.append(file_dict)
    return standard_data_clean


def remove_unnecessary_standard_fields(standard_data, sample_fields, ignore_van):
    if ignore_van:
        vana_data_list = [entry for entry in standard_data if entry["sample_type"] == "vana"]
        vana_fields = [field_dict.get(entry["field"], entry["field"]) for entry in vana_data_list]
        nicr_fields = list(sample_fields)
        empty_fields = set(vana_fields + list(sample_fields))
        nicr_data_list = [
            entry
            for entry in standard_data
            if entry["sample_type"] == "nicr" and field_dict.get(entry["field"], entry["field"]) in nicr_fields
        ]
        empty_data_list = [
            entry
            for entry in standard_data
            if entry["sample_type"] == "empty" and field_dict.get(entry["field"], entry["field"]) in empty_fields
        ]
        standard_data_clean = vana_data_list + nicr_data_list + empty_data_list
    else:
        standard_data_clean = [entry for entry in standard_data if field_dict.get(entry["field"], entry["field"]) in sample_fields]
    return standard_data_clean


def get_bank_positions(sample_data, rounding_limit=0.05):
    new_arr = []
    inside = False
    banks = set(entry["det_rot"] for entry in sample_data)
    for bank in banks:
        for compare in new_arr:
            if abs(compare - bank) < rounding_limit:
                inside = True
                break
            inside = False
        if not inside:
            new_arr.append(bank)
    return new_arr


def get_sample_fields_each_bank(sample_data):
    det_banks = get_bank_positions(sample_data)
    angle_and_fields_data = {}
    for det_bank in det_banks:
        fields = []
        for file_dict in sample_data:
            if det_bank == file_dict["det_rot"]:
                field = field_dict.get(file_dict["field"], file_dict["field"])
                fields.append(field)
        angle_and_fields_data[det_bank] = set(fields)
    return angle_and_fields_data


def _get_path_string(dataset, sample_name):
    path = dataset[sample_name].pop("path")
    return f"'path': '{path}',\n"


def _get_sample_string(sample_name):
    return f"'{sample_name:s}': {{"


def _get_field_string(fields, line_indent):
    field_items = sorted(fields.items())
    spacer = "".rjust(line_indent)
    string_list = [(f"{spacer}'{key:s}': {value}") for key, value in field_items]
    return ",\n".join(string_list)
