# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantid workbench.

import unittest

import matplotlib
matplotlib.use("Agg")  # noqa
import matplotlib.pyplot as plt
from copy import copy

from mantid.simpleapi import CreateWorkspace
from workbench.plotting.plotscriptgenerator import generate_script
from workbench.plotting.plotscriptgenerator.test.test_plotscriptgeneratorlines import ERRORBAR_KWARGS


class PlotScriptGeneratorTest(unittest.TestCase):

    @classmethod
    def setUpClass(cls):
        cls.test_ws = CreateWorkspace(
            DataX=[10, 20, 30, 10, 20, 30],
            DataY=[2, 3, 4, 5],
            DataE=[1, 2, 3, 4],
            NSpec=2,
            OutputWorkspace='test_ws')

    def setUp(self):
        self.fig = plt.figure()
        self.ax = self.fig.add_subplot(1, 1, 1, projection='mantid')

    def tearDown(self):
        plt.close()

    def test_generate_script_returns_correct_string_for_single_errorbar_plot(self):
        kwargs = copy(ERRORBAR_KWARGS)
        self.ax.errorbar(self.test_ws, **kwargs)
        expected_string = """fig = plt.figure(dpi=100.0, figsize=(6.4, 4.8))
ax = fig.add_subplot(1, 1, 1, frame_on=True, label='', projection='mantid', sharex=None, sharey=None, title='', visible=True, xlabel='', xlim=(14.5, 25.5), xscale='linear', ylabel='', ylim=(0.07999999999999997, 0.52), yscale='linear')
ax.errorbar(test_ws, alpha=0.5, barsabove=True, capsize=1.6, capthick=1.2, color='r', distribution=False, drawstyle='steps', ecolor='#ff0000', elinewidth=1.5, errorevery=1, fillstyle='left', label='test label', linestyle='--', linewidth=1.1, marker='o', markeredgecolor='g', markerfacecolor='y', markerfacecoloralt='k', markersize=1.3, markevery=2, solid_capstyle='butt', solid_joinstyle='round', specNum=1, visible=False, zorder=1.4)
plt.show()"""
        self.assertEqual(expected_string, generate_script(self.fig))

    def test_generate_script_returns_correct_number_of_lines_for_overplot(self):
        self.ax.plot(self.test_ws, specNum=1)
        self.ax.errorbar(self.test_ws, specNum=2)
        output = generate_script(self.fig)
        num_lines = len(output.split('\n'))
        err_message = ("Expected to output 5 lines, found {}.\nOutput:\n{}"
                       "".format(num_lines, output))
        self.assertEqual(5, num_lines, msg=err_message)


if __name__ == '__main__':
    unittest.main()
