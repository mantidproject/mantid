# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantid workbench.

import unittest

from ast import parse
from collections import OrderedDict
from numpy import array

from unittest.mock import patch, Mock
from workbench.plotting.plotscriptgenerator.utils import (
    convert_args_to_string,
    get_plotted_workspaces_names,
    clean_variable_name,
    generate_workspace_retrieval_commands,
)


class PlotScriptGeneratorUtilsTest(unittest.TestCase):
    def test_convert_args_to_string_returns_correct_string(self):
        kwargs_dict = OrderedDict(
            {"key0": "val0", "key1": [2, "str"], "key2": 1, "key3": {"a": 1.1, "b": {"c": ["str2", 1.1]}}, "ndarray": array([1.1, 1.2])}
        )
        expected_str = "key0='val0', key1=[2, 'str'], key2=1, key3={'a': 1.1, 'b': {'c': ['str2', 1.1]}}, ndarray=[1.1, 1.2]"
        self.assertEqual(expected_str, convert_args_to_string(None, kwargs_dict))

    def test_get_plotted_workspace_names_returns_list_of_workspace_names(self):
        mock_axes = [Mock(tracked_workspaces={"test_ws": None}), Mock(tracked_workspaces={"test_ws1": None})]
        mock_fig = Mock(get_axes=lambda: mock_axes)
        plotted_workspaces = get_plotted_workspaces_names(mock_fig)
        self.assertEqual(["test_ws", "test_ws1"], sorted(plotted_workspaces))

    @patch("workbench.plotting.plotscriptgenerator.utils.get_plotted_workspaces_names")
    def test_generate_workspace_retrieval_commands_does_not_repeat_retrieval_if_workspace_plotted_twice(self, mock_plotted_name):
        mock_plotted_name.return_value = ["ws1", "ws2", "ws1", "ws-3"]
        expected_output = sorted(
            [
                "from mantid.api import AnalysisDataService as ADS\n",
                "ws1 = ADS.retrieve('ws1')",
                "ws2 = ADS.retrieve('ws2')",
                "ws_3 = ADS.retrieve('ws-3')",
            ]
        )
        self.assertEqual(expected_output, sorted(generate_workspace_retrieval_commands(None)))

    def test_clean_variable_name_returns_valid_variable_names(self):
        invalid_variable_names = ["2GEM_log", "GEM-log", "GEM*log", "GEM+log"]
        for var_name in invalid_variable_names:
            clean_name = clean_variable_name(var_name)
            try:
                parse("{} = None".format(clean_name))
            except (SyntaxError, TypeError, ValueError):
                msg = "Invalid variable name: {}".format(clean_name)
                self.fail(msg)


if __name__ == "__main__":
    unittest.main()
