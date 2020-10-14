# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +


def sequoia(name):
    name = name.strip().upper()
    packs = (['B{}'.format(i) for i in range(1, 37+1)]
             + ['C{}'.format(i) for i in range(1, 24+1)]
             + ['C25T', 'C26T', 'C25B', 'C26B']
             + ['C{}'.format(i) for i in range(27, 37+1)]
             + ['D{}'.format(i) for i in range(1, 37+1)]
             )
    start_index = 38
    return start_index + packs.index(name)
