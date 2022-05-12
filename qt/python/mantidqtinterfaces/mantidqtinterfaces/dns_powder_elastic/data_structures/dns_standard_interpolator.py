# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
"""
Class which interpolates DNS standard data to bank positions
"""
import os
import numpy as np

from mantidqtinterfaces.dns_powder_tof.data_structures.dns_file import DNSFile
from mantidqtinterfaces.dns_powder_tof.helpers.file_processing import \
    get_path_and_prefix, create_dir


def read_standard_file(path, number):
    path, prefix = get_path_and_prefix(path)
    filename = f'{prefix}_{number:06d}.d_dat'
    dnsfile = DNSFile(path, filename)
    int_arr = np.array([dnsfile.det_rot, dnsfile.timer, dnsfile.monitor])
    int_arr = np.append(int_arr, dnsfile.counts[:, 0])
    return int_arr


def create_array(path, filenumbers):
    arr = np.asarray([])
    filenumb_dict = {}
    for number in filenumbers:
        new = read_standard_file(path, number)
        filenumb_dict[new[0]] = number  # det_rot dictionary
        arr = np.append(arr, new)
    arr = np.reshape(np.asarray(arr), (-1, 27))
    arr = np.sort(arr, axis=0)
    return [arr, filenumb_dict]


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


def closest_filenumber(x, filenumb_dict):
    res = filenumb_dict.get(x) or filenumb_dict[min(
        filenumb_dict.keys(), key=lambda key: abs(key - x))]
    return res


def write_inp_file(arr, path, filenumb_dict, filenumber, prefix):
    filenumbers = []
    for line in arr:
        number = int(line[0])
        number = closest_filenumber(number, filenumb_dict)
        dirname, propnb = get_path_and_prefix(path)
        filename = f'{propnb}_{number:06d}.d_dat'
        dnsfile = DNSFile(dirname, filename)
        countarray = np.zeros((24, 1), dtype=int)
        countarray[:, 0] = line[3:]
        dnsfile.counts = countarray
        dnsfile.det_rot = line[0]
        dnsfile.timer = line[1]
        dnsfile.monitor = int(line[2])
        newfilename = f'{prefix}_{filenumber:06d}.d_dat'
        dnsfile.write(dirname + '/interp/', newfilename)
        filenumbers.append(filenumber)
        filenumber += 1
    return filenumbers


def interpolate_standard(standard_data, bank_positions, scriptname):
    new_dict = {}
    error = False
    for sample_type in standard_data.keys():
        path = standard_data[sample_type]['path']
        newpath = os.path.dirname(path).rstrip("/\\") + '/interp/'
        prefix = scriptname[0:-3] + '_ip_' + sample_type
        new_dict[sample_type] = {}
        new_dict[sample_type]['path'] = newpath + prefix
        filenumber = 0
        create_dir(newpath)
        for field in [
            x for x in standard_data[sample_type].keys() if x != 'path'
        ]:
            numbers = standard_data[sample_type][field]
            arr, filenumb_dict = (create_array(path, numbers))
            arr = average_array(arr)
            intp = interp(arr, bank_positions)
            filenumbers = write_inp_file(intp, path, filenumb_dict, filenumber,
                                         prefix)
            filenumber = filenumbers[-1]
            new_dict[sample_type][field] = filenumbers
            filenumber += 1
    return [new_dict, error]
