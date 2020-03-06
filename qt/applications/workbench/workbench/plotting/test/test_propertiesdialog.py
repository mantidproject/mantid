# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantid workbench.
#
from __future__ import (absolute_import, division, print_function,
                        unicode_literals)

# system imports
import unittest

# third-party library imports
import matplotlib

matplotlib.use('AGG')  # noqa
import matplotlib.pyplot as plt
from mantidqt.utils.qt.testing import start_qapplication
from workbench.plotting.propertiesdialog import XAxisEditor, YAxisEditor


@start_qapplication
class PropertiesDialogTest(unittest.TestCase):

    # Success tests
    def test_axis_editor_initialised_with_correct_values(self):
        # make figure
        fig, ax = plt.subplots(1, 1)
        ax.plot([1, 2, 3], [1, 10, 100], 'o')
        # set properties that can be accessed via the axes menu
        ax.xaxis.grid(True)
        ax.set(xlim=[0, 4], ylim=[1e-3, 1e3], yscale='log')
        # get an AxisEditor object for x/y axes
        xEditor = XAxisEditor(fig.canvas, ax)
        yEditor = YAxisEditor(fig.canvas, ax)
        # test grid visibility
        self.assertEqual(xEditor._memento.grid, ax.xaxis._gridOnMajor)
        self.assertEqual(yEditor._memento.grid, ax.yaxis._gridOnMajor)
        # test limits
        self.assertEqual(xEditor._memento.min, ax.get_xlim()[0])
        self.assertEqual(xEditor._memento.max, ax.get_xlim()[1])
        self.assertEqual(yEditor._memento.min, ax.get_ylim()[0])
        self.assertEqual(yEditor._memento.max, ax.get_ylim()[1])
        # test scale
        self.assertEqual(xEditor._memento.log, False)
        self.assertEqual(yEditor._memento.log, True)


if __name__ == '__main__':
    unittest.main()
