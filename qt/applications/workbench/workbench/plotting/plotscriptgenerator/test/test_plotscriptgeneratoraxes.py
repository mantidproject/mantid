# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantid workbench.

from __future__ import (absolute_import, unicode_literals)

import unittest

from mantid.py3compat.mock import Mock
from workbench.plotting.plotscriptgenerator.axes import (generate_axis_limit_commands,
                                                         generate_axis_label_commands)


class PlotGeneratorAxisTest(unittest.TestCase):

    def test_generate_axis_limits_commands_returns_correct_commands(self):
        mock_ax = Mock(get_xlim=lambda: (0.5, 1), get_ylim=lambda: (2, 10))
        expected = ["set_xlim([0.5, 1])", "set_ylim([2, 10])"]
        actual = generate_axis_limit_commands(mock_ax)
        self.assertEqual(expected, actual)

    def test_generate_axis_label_commands_only_returns_commands_for_labels_that_are_set(self):
        mock_ax = Mock(get_xlabel=lambda: '', get_ylabel=lambda: 'y')
        expected = ["set_ylabel('y')"]
        self.assertEqual(expected, generate_axis_label_commands(mock_ax))

    def test_generate_axis_label_commands_returns_empty_list_when_no_labels_set(self):
        mock_ax = Mock(get_xlabel=lambda: '', get_ylabel=lambda: '')
        self.assertEqual([], generate_axis_label_commands(mock_ax))

    def test_generate_axis_label_commands_returns_x_and_y_command_if_both_labels_set(self):
        mock_ax = Mock(get_xlabel=lambda: "X", get_ylabel=lambda: "Y")
        expected = ["set_xlabel('X')", "set_ylabel('Y')"]
        actual = generate_axis_label_commands(mock_ax)
        self.assertEqual(expected, actual)

    def test_generate_axis_label_commands_returns_only_x_command_if_y_label_not_set(self):
        mock_ax = Mock(get_xlabel=lambda: "X", get_ylabel=lambda: "")
        expected = ["set_xlabel('X')"]
        actual = generate_axis_label_commands(mock_ax)
        self.assertEqual(expected, actual)

    def test_generate_axis_label_commands_returns_empty_list_if_no_labels_set(self):
        mock_ax = Mock(get_xlabel=lambda: "", get_ylabel=lambda: "")
        actual = generate_axis_label_commands(mock_ax)
        self.assertEqual([], actual)


if __name__ == '__main__':
    unittest.main()
