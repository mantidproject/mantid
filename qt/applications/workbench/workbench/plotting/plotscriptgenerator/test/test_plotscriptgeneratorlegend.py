# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantid workbench.

import unittest

import matplotlib
matplotlib.use("Agg")  # noqa
import matplotlib.pyplot as plt

from mantid.simpleapi import CreateWorkspace
from workbench.plotting.plotscriptgenerator.legend import *

DEFAULT_TITLE_FONT_KWARGS = {
    'font': mpl_default_kwargs['title_font'],
    'color': mpl_default_kwargs['title_color']
}

DEFAULT_LABEL_FONT_KWARGS = {
    'font': mpl_default_kwargs['entries_font'],
    'color': mpl_default_kwargs['entries_color']
}

TEST_FONT_KWARGS = {
    'font': 'Arial',
    'color': '#ff0000'
}


class PlotScriptGeneratorLegendTest(unittest.TestCase):

    @classmethod
    def setUpClass(cls):
        cls.test_ws = CreateWorkspace(DataX=[10, 20, 30, 10, 20, 30],
                                      DataY=[2, 3, 4, 5],
                                      DataE=[1, 2, 3, 4],
                                      NSpec=2,
                                      OutputWorkspace='test_ws')

    def setUp(self):
        fig = plt.figure()
        self.ax = fig.add_subplot(1, 1, 1, projection='mantid')
        self.ax.plot(self.test_ws, wkspIndex=0)
        self.ax.plot(self.test_ws, wkspIndex=1)

    def tearDown(self):
        plt.close()

    def test_title_font_commands(self):
        """
        Check that the title font properties are correct when set to non-default values.
        """
        legend = self.ax.legend(title="Legend title")
        title = legend.get_title()
        title.set_fontname(TEST_FONT_KWARGS['font'])
        title.set_color(TEST_FONT_KWARGS['color'])
        title_font_commands = generate_title_font_commands(legend, "legend")

        # There should be two lines of commands, one to set the font name, and the other to set the colour.
        self.assertEqual(2, len(title_font_commands))
        # Default arguments should not appear in the list of commands.
        for value in DEFAULT_TITLE_FONT_KWARGS.values():
            self.assertFalse(any(str(value) in command for command in title_font_commands))
        # The new values should all appear in the list of commands.
        for value in TEST_FONT_KWARGS.values():
            self.assertTrue(any(str(value) in command for command in title_font_commands))

    def test_label_font_commands(self):
        """
        Check that the label font commands are correct when set to non-default values.
        """
        legend = self.ax.legend()
        for label in legend.get_texts():
            label.set_fontname(TEST_FONT_KWARGS['font'])
            label.set_color(TEST_FONT_KWARGS['color'])
        label_font_commands = generate_label_font_commands(legend, "legend")
        # There should be two lines of commands, one to set the font name, and the other to set the colour.
        self.assertEqual(2, len(label_font_commands))
        # Default arguments should not appear in the list of commands.
        for value in DEFAULT_TITLE_FONT_KWARGS.values():
            self.assertFalse(any(str(value) in command for command in label_font_commands))
        # The new values should all appear in the list of commands.
        for value in TEST_FONT_KWARGS.values():
            self.assertTrue(any(str(value) in command for command in label_font_commands))

    def test_legend_kwargs(self):
        """
        Set some legend properties, making sure they're not default. Check that the correct properties are returned.
        """
        test_kwargs = {
            'background_color': '#ff0000',
            'edge_color': '#ff0000',
            'transparency': mpl_default_kwargs['transparency'] * 0.5,
            'entries_size': 20,
            'title_size': 20,
            'columns': mpl_default_kwargs['columns'] + 1,
            'markers': mpl_default_kwargs['markers'] + 1,
            'marker_position': "Right of Entries",
            'box_visible': not mpl_default_kwargs['box_visible'],
            'round_edges': not mpl_default_kwargs['round_edges'],
            'shadow': not mpl_default_kwargs['shadow'],
            'title': 'Test title',
            'border_padding': mpl_default_kwargs['border_padding'] * 0.5,
            'label_spacing': mpl_default_kwargs['label_spacing'] * 0.5,
            'marker_size': mpl_default_kwargs['marker_size'] + 2,
            'marker_label_padding': mpl_default_kwargs['marker_label_padding'] * 0.5,
            'column_spacing': mpl_default_kwargs['column_spacing'] * 0.5
        }
        # Convert the mantid kwargs to mpl kwargs.
        mpl_test_kwargs = get_mpl_kwargs(test_kwargs)

        legend = self.ax.legend(**mpl_test_kwargs)
        legend_kwargs = get_legend_command_kwargs(legend)
        # The number of properties should be equal to the number of non-default properties.
        self.assertEqual(len(test_kwargs), len(legend_kwargs))

        # The new values should all be present. We can compare with our original test kwargs.
        for key, value in legend_kwargs.items():
            self.assertEqual(value, mpl_test_kwargs[key])

    def test_default_legend_properties(self):
        """
        Leave all legend properties as defaults and check that generators return empty lists.
        """
        legend = self.ax.legend()
        legend_kwargs = get_legend_command_kwargs(legend)
        label_font_commands = generate_label_font_commands(legend, "legend")
        title_font_commands = generate_title_font_commands(legend, "legend")

        self.assertEqual(0, len(legend_kwargs))
        self.assertEqual(0, len(label_font_commands))
        self.assertEqual(0, len(title_font_commands))


if __name__ == '__main__':
    unittest.main()
