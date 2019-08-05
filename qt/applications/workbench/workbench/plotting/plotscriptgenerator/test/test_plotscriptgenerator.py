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
from matplotlib.axes import Axes

from mantid.py3compat.mock import Mock
from mantid.simpleapi import CreateWorkspace
from workbench.plotting.plotscriptgenerator import generate_script, DEFAULT_CONTENT
from workbench.plotting.plotscriptgenerator.test.test_plotscriptgeneratorlines import ERRORBAR_KWARGS


EXAMPLE_SCIRPT = (
    "{}"
    "\n"
    "test_ws = CreateWorkspace(OutputWorkspace='test_ws', "
    "DataX='10,20,30,10,20,30', DataY='2,3,4,5', DataE='1,2,3,4', NSpec=2)\n"
    "\n"
    "fig = plt.figure(dpi=100.0, figsize=(6.4, 4.8))\n"
    "ax = fig.add_subplot(1, 1, 1, frame_on=True, label='', projection='mantid', "
    "sharex=None, sharey=None, title='', visible=True, xlabel='', "
    "xlim=(14.5, 25.5), xscale='linear', ylabel='', "
    "ylim=(0.07999999999999997, 0.52), yscale='linear')\n"
    "ax.errorbar(test_ws, alpha=0.5, barsabove=True, capsize=1.6, capthick=1.2, "
    "color='r', distribution=False, drawstyle='steps', ecolor='#ff0000', "
    "elinewidth=1.5, errorevery=1, fillstyle='left', label='test label', "
    "linestyle='--', linewidth=1.1, marker='o', markeredgecolor='g', "
    "markerfacecolor='y', markerfacecoloralt='k', markersize=1.3, markevery=2, "
    "solid_capstyle='butt', solid_joinstyle='round', specNum=1, visible=False, "
    "zorder=1.4)\n"
    "plt.show()".format(DEFAULT_CONTENT))


class PlotScriptGeneratorTest(unittest.TestCase):

    @classmethod
    def setUpClass(cls):
        cls.test_ws = CreateWorkspace(
            DataX=[10, 20, 30, 10, 20, 30],
            DataY=[2, 3, 4, 5],
            DataE=[1, 2, 3, 4],
            NSpec=2,
            OutputWorkspace='test_ws')

    def tearDown(self):
        plt.close()

    def test_generate_script_returns_correct_string_for_single_errorbar_plot(self):
        fig = plt.figure()
        ax = fig.add_subplot(1, 1, 1, projection='mantid')
        kwargs = copy(ERRORBAR_KWARGS)
        ax.errorbar(self.test_ws, **kwargs)
        self.assertEqual(EXAMPLE_SCIRPT, generate_script(fig))

    def test_generate_script_returns_correct_number_of_lines_for_overplot(self):
        fig = plt.figure()
        ax = fig.add_subplot(1, 1, 1, projection='mantid')
        ax.plot(self.test_ws, specNum=1)
        ax.errorbar(self.test_ws, specNum=2)
        output = generate_script(fig)
        num_lines = len(output.split('\n'))
        # Expect 11 lines from DEFAULT_CONTENT, 2 for loading workspace and 5 for plotting
        expected_num_lines = 18
        err_message = ("Expected to output {} lines, found {}.\nOutput:\n{}"
                       "".format(expected_num_lines, num_lines, output))
        self.assertEqual(expected_num_lines, num_lines, msg=err_message)

    def test_generate_script_returns_None_if_no_MantidAxes_in_figure(self):
        mock_fig = Mock(get_axes=lambda: [Mock(spec=Axes)])
        self.assertEqual(None, generate_script(mock_fig))


if __name__ == '__main__':
    unittest.main()
