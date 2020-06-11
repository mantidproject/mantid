# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantid workbench.

import unittest

import matplotlib
matplotlib.use("Agg")  # noqa
import matplotlib.pyplot as plt
from copy import copy

from mantid.simpleapi import CreateSampleWorkspace
from workbench.plotting.plotscriptgenerator.colorfills import (CFILL_NAME, _get_plot_command_kwargs_from_colorfill,
                                                               generate_plot_2d_command)
from workbench.plotting.plotscriptgenerator.utils import convert_args_to_string

CFILL_KWARGS = {
    'aspect': 'auto',
    'cmap': 'viridis',
    'origin': 'lower',
    'label': 'test label',
    'zorder': 1,
}

MANTID_ONLY_KWARGS = {'distribution': False}


class PlotScriptGeneratorColorFillsTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.test_ws = CreateSampleWorkspace(OutputWorkspace='PlotScriptGeneratorColorFillsTest_test_ws')

    def setUp(self):
        fig, self.ax = plt.subplots(subplot_kw={'projection': 'mantid'})

    def tearDown(self):
        plt.close()

    def test_get_plot_command_kwargs_from_colourfill_returns_dict_with_correct_properties(self):
        cfill = self.ax.imshow(self.test_ws, **CFILL_KWARGS)
        plot_commands_dict = _get_plot_command_kwargs_from_colorfill(cfill)
        for key, value in CFILL_KWARGS.items():
            self.assertEqual(value, plot_commands_dict[key])

    def test_generate_plot_2d_command_returns_correct_string_for_colorfill(self):
        kwargs = copy(CFILL_KWARGS)
        kwargs.update(MANTID_ONLY_KWARGS)
        cfill = self.ax.imshow(self.test_ws, **CFILL_KWARGS)
        output = generate_plot_2d_command(cfill)
        arg_string = convert_args_to_string([self.test_ws], kwargs)
        expected_command = [f"{CFILL_NAME} = axes.imshow({arg_string})",
                            f"{CFILL_NAME}.set_norm(plt.Normalize(vmin=",
                            f"cbar = fig.colorbar({CFILL_NAME}, ax=[axes], pad=0.06)"]
        self.assertEqual(len(expected_command), len(output))
        for line_no in range(len(output)):
            # only compare to the length of the expected output
            # as one line is truncated on purpose
            self.assertEqual(expected_command[line_no], output[line_no][:len(expected_command[line_no])])


if __name__ == '__main__':
    unittest.main()
