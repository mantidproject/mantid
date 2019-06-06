# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantid workbench.

import unittest

from matplotlib import use as mpl_use
mpl_use('Agg')  # noqa
from matplotlib.pyplot import figure

from mantidqt.widgets.plotconfigdialog.curvestabwidget.errorbarstabwidget import ErrorbarsProperties


class errorbarPropertiesTest(unittest.TestCase):

    @classmethod
    def setUpClass(cls):
        fig = figure()
        cls.ax = fig.add_subplot(111)

    def test_errorbar_curve_properties_read_correctly(self):
        plot_args = {'fmt': '--o', 'ecolor': 'r', 'elinewidth': 0.1,
                     'capsize': 1.1, 'capthick': 1.2, 'errorevery': 1}
        expected = {'capsize': 1.1, 'cap_thickness': 1.2, 'color': '#ff0000',
                    'width': 0.1, 'hide': False}
        curve = self.ax.errorbar([0, 1], [0, 1], [0.1, 0.1], [0.1, 0.1],
                                 **plot_args)
        err_props = ErrorbarsProperties.from_container(curve)
        for prop, value in expected.items():
            self.assertEqual(value, getattr(err_props, prop))

    def test_errorbar_curve_with_no_line_properties_read_correctly(self):
        plot_args = {'fmt': 'none', 'ecolor': 'r', 'elinewidth': 0.1,
                     'capsize': 1.1, 'capthick': 1.2, 'errorevery': 1}
        expected = {'capsize': 1.1, 'cap_thickness': 1.2, 'color': '#ff0000',
                    'width': 0.1, 'hide': False}
        curve = self.ax.errorbar([0, 1], [0, 1], [0.1, 0.1], [0.1, 0.1],
                                 **plot_args)
        err_props = ErrorbarsProperties.from_container(curve)
        for prop, value in expected.items():
            self.assertEqual(value, getattr(err_props, prop))

    def test_errorbar_curve_with_no_caps_properties_read_correctly(self):
        plot_args = {'fmt': 'none', 'ecolor': 'r', 'elinewidth': 0.1,
                     'capsize': 0, 'capthick': 0, 'errorevery': 1}
        expected = {'color': '#ff0000', 'width': 0.1, 'hide': False}
        curve = self.ax.errorbar([0, 1], [0, 1], [0.1, 0.1], [0.1, 0.1],
                                 **plot_args)
        err_props = ErrorbarsProperties.from_container(curve)
        for prop, value in expected.items():
            self.assertEqual(value, getattr(err_props, prop))

        self.assertFalse(hasattr(err_props, 'capsize'))
        self.assertFalse(hasattr(err_props, 'cap_thickness'))

    def test_returns_none_if_not_passed_errorbar_container(self):
        self.assertEqual(None, ErrorbarsProperties.from_container([]))


if __name__ == '__main__':
    unittest.main()
