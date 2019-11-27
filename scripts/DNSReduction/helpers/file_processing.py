# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
"""
DNS file helpers
"""
from __future__ import (absolute_import, division, print_function)
import glob as glob


def filter_filenames(alldatafiles, start, end):
    """
    Filter datafilenames to the range given
    """
    filtered = []
    for filename in alldatafiles:
        number = int(filename.split('_')[-2][:-2])
        if number <= end and number >= start:
            filtered.append(filename)
    return filtered


def return_filelist(datadir):
    """
    Return list of names of dnsfiles in datadir
    """
    filelist = glob.glob(datadir + "/*_[0-9][0-9][0-9][0-9][0-9][0-9].d_dat")
    for i in range(len(filelist)):
        filelist[i] = filelist[i].replace('\\', '/')
    return filelist
