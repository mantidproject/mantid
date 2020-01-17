# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantid package
# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
from __future__ import absolute_import, division

import unittest

from mantid.plots.utility import legend_set_draggable
from mantid.py3compat.mock import create_autospec
from matplotlib.legend import Legend


class UtilityTest(unittest.TestCase):

    def test_legend_set_draggable(self):
        legend = create_autospec(Legend)
        args = (None, False, 'loc')
        legend_set_draggable(legend, *args)

        if hasattr(Legend, 'set_draggable'):
            legend.set_draggable.assert_called_with(*args)
        else:
            legend.draggable.assert_called_with(*args)


if __name__ == '__main__':
    unittest.main()
