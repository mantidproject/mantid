# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +

"""
Class which interpolates DNS standard data to bank positions.
"""

import os
import numpy as np

from mantidqtinterfaces.dns_powder_tof.data_structures.dns_file import DNSFile
from mantidqtinterfaces.dns_powder_tof.helpers.file_processing import \
    get_path_and_prefix, create_dir


def read_standard_file(path, number):
    path, prefix = get_path_and_prefix(path)
    filename = f'{prefix}_{number:06d}.d_dat'
    dns_file = DNSFile(path, filename)
    int_arr = np.array([dns_file.det_rot, dns_file.timer, dns_file.monitor])
    int_arr = np.append(int_arr, dns_file.counts[:, 0])
    return int_arr


def create_array(path, file_numbers):
    arr = np.asarray([])
    file_numb_dict = {}
    for number in file_numbers:
        new = read_standard_file(path, number)
        file_numb_dict[new[0]] = number  # det_rot dictionary
        arr = np.append(arr, new)
    arr = np.reshape(np.asarray(arr), (-1, 27))
    arr = np.sort(arr, axis=0)
    return [arr, file_numb_dict]


def average_array(arr, rounding_limit=0.05):
    new_arr = []
    number = None
    inside = False
    for line in arr:
        for number, compare in enumerate(new_arr):
            if abs(compare[0] - line[0]) < rounding_limit:
                inside = True
                break
            inside = False
        if inside:
            new_arr[number][1:] = new_arr[number][1:] + line[1:]
        else:
            new_arr.append(line)
    return np.stack(new_arr)


def interp(arr, x):
    alle = []
    for det_number in range(27):
        alle.append(
            np.round(np.interp(x=x, xp=arr[:, 0], fp=arr[:, det_number]), 2))
    new = np.vstack(alle)
    new[0] = x
    new = np.transpose(new)
    return new


def closest_file_number(x, file_numb_dict):
    res = file_numb_dict.get(x) or file_numb_dict[min(
        file_numb_dict.keys(), key=lambda key: abs(key - x))]
    return res


def write_inp_file(arr, path, file_numb_dict, file_number, prefix):
    file_numbers = []
    for line in arr:
        number = int(line[0])
        number = closest_file_number(number, file_numb_dict)
        dir_name, prop_nb = get_path_and_prefix(path)
        filename = f'{prop_nb}_{number:06d}.d_dat'
        dns_file = DNSFile(dir_name, filename)
        count_array = np.zeros((24, 1), dtype=int)
        count_array[:, 0] = line[3:]
        dns_file.counts = count_array
        dns_file.det_rot = line[0]
        dns_file.timer = line[1]
        dns_file.monitor = int(line[2])
        new_filename = f'{prefix}_{file_number:06d}.d_dat'
        dns_file.write(dir_name + '/interp/', new_filename)
        file_numbers.append(file_number)
        file_number += 1
    return file_numbers


def interpolate_standard(standard_data, bank_positions, script_name):
    new_dict = {}
    error = False
    for sample_type in standard_data.keys():
        path = standard_data[sample_type]['path']
        new_path = os.path.dirname(path).rstrip("/\\") + '/interp/'
        prefix = script_name[0:-3] + '_ip_' + sample_type
        new_dict[sample_type] = {}
        new_dict[sample_type]['path'] = new_path + prefix
        file_number = 0
        create_dir(new_path)
        for field in [
            x for x in standard_data[sample_type].keys() if x != 'path'
        ]:
            numbers = standard_data[sample_type][field]
            arr, file_numb_dict = (create_array(path, numbers))
            arr = average_array(arr)
            intp = interp(arr, bank_positions)
            file_numbers = write_inp_file(intp, path, file_numb_dict, file_number,
                                          prefix)
            file_number = file_numbers[-1]
            new_dict[sample_type][field] = file_numbers
            file_number += 1
    return [new_dict, error]
