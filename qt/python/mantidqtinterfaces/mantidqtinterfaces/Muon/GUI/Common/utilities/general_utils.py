# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import numpy as np


def round_significant_figures(value, sf=3):
    return ('{:.{}g}').format(float(value), str(sf))


def round_value(value, num_dp=3):
    rounded = ('{:.{}f}').format(float(value), str(num_dp))
    if float(rounded) != 0.0:
        return rounded
    else:
        return round_significant_figures(value, num_dp)


def round_to_min_whole_number_or_sf(value, sf=3):
    # this will give whole number if sf would truncate the whole number
    number = float(value)
    if number > np.power(10,sf):
        return round_value(value, 0)
    else:
        return round_significant_figures(value, sf)
