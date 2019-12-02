# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
"""
Interpolation of DNS elastic standard data to sample data positions
not in use atm
"""

from __future__ import (absolute_import, division, print_function)
import numpy as np

field_dict = {
    'x7_sf': 'x_sf',
    'x20_sf': 'x_sf',
    '-x7_sf': 'minus_x_sf',
    '-x20_sf': 'minus_x_sf',
    'x7_nsf': 'x_nsf',
    'x20_nsf': 'x_nsf',
    '-x7_nsf': 'minus_x_nsf',
    '-x20_nsf': 'minus_x_nsf',
    'y7_sf': 'y_sf',
    'y20_sf': 'y_sf',
    '-y7_sf': 'minus_y_sf',
    '-y20_sf': 'minus_y_sf',
    'y7_nsf': 'y_nsf',
    'y20_nsf': 'y_nsf',
    '-y7_nsf': 'minus_y_nsf',
    '-y20_nsf': 'minus_y_nsf',
    'z7_sf': 'z_sf',
    'z20_sf': 'z_sf',
    '-z7_sf': 'minus_z_sf',
    '-z20_sf': 'minus_z_sf',
    'z7_nsf': 'z_nsf',
    'z20_nsf': 'z_nsf',
    '-z7_nsf': 'minus_z_nsf',
    '-z20_nsf': 'minus_z_nsf',
}

number_dict = {entry['filenumber']: entry for entry in fulldata}


def create_standard():
    dataset = {}
    for entry in fulldata:
        if entry['samplename'] not in ['vana', 'nicr', 'leer', 'empty']:
            datatype = 'sample'
        elif entry['samplename'] == 'leer':  ## compatibility with old dnsplot
            datatype = 'empty'
        else:
            datatype = entry['samplename']
        field = field_dict.get(entry['field'], entry['field'])
        datapath = entry['filename'].replace(
            '_' + str(entry['filenumber']) + '.d_dat', '')
        if datatype in dataset.keys():
            if field in dataset[datatype].keys():
                if datapath in dataset[datatype][field].keys():
                    dataset[datatype][field][datapath].append(
                        entry['filenumber'])
                else:
                    dataset[datatype][field][datapath] = [entry['filenumber']
                                                         ]  #
            else:
                dataset[datatype][field] = {}
                dataset[datatype][field][datapath] = [entry['filenumber']]
        else:
            dataset[datatype] = {}
            dataset[datatype][field] = {}
            dataset[datatype][field][datapath] = [entry['filenumber']]
    #print(dataset)
    return dataset


data = create_standard()

#### reading vanadium data
vanadata = {}
arr = []
for mykey, values in vanadata['sample']['x_sf'].items():
    for number in values:
        filename = '{}_{}.d_dat'.format(mykey, number)
        with open(filename, 'r') as f:
            txt = f.readlines()
        del f
        det_rot = float(txt[13][-15:-5].strip())
        time = float(txt[56][-15:-5].strip())
        monitor = int(txt[57][-15:-1].strip())
        int_list = [det_rot, time, monitor]
        for i in range(24):
            int_list.append(int(txt[74 + i][3:-1].strip()))
        arr.append(int_list)
        del txt
arr = np.reshape(np.asarray(arr), (-1, 27))
arr = np.sort(arr, axis=0)

#### averaging standard files which are closer together than 0.05
### simply starts rom lowest value, if you choose rounding limit to large
### all will be put in 1 bin
rounding_limit = 0.05
new_arr = []
dividers = []
inside = False
for line in arr:
    for number, compare in enumerate(new_arr):
        if abs(compare[0] - line[0]) < rounding_limit:
            inside = True
            break
        else:
            inside = False
    if inside:
        new_arr[number] = new_arr[number] + line
        dividers[number] += 1
    else:
        new_arr.append(line)
        dividers.append(1)
for number, divisor in enumerate(dividers):
    new_arr[number][0] = new_arr[number][0] / divisor
new_arr = np.stack(new_arr)
print(new_arr)

## interpolating new intensities
x = [-10.1, -9, -8, -7.4, -6.9, -6, -5.25, -4.5]
alle = []
for det_number in range(27):
    alle.append(
        np.round(np.interp(x=x, xp=arr[:, 0], fp=arr[:, det_number]), 2))
new = np.vstack(alle)
new[0] = x
new = np.transpose(new)  #np.reshape(alle,(len(x),-1))
print(new)
