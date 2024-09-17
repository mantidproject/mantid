# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2023 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +

"""
DNS unit converters.
"""

from mantid.geometry import UnitCell


def d_spacing_from_lattice(a, b, c, alpha, beta, gamma, hkl):
    unit_cell = UnitCell(a, b, c, alpha, beta, gamma)
    h, k, l = hkl
    d = round(UnitCell.d(unit_cell, h, k, l), 5)
    return d
