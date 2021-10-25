# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +


def round_value(value, num_dp=3):
    rounded = ('{:.'+str(num_dp)+'f}').format(float(value))
    if float(rounded) != 0.0:
        return rounded
    else:
        return ('{:.'+str(num_dp)+'g}').format(float(value))
