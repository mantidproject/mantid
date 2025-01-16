# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantid workbench.
import ast
import re
import json
import unittest
from unittest.mock import Mock

from matplotlib.backend_bases import FigureManagerBase

from mantidqt.widgets.fitpropertybrowser import FitPropertyBrowser
from workbench.plotting.plotscriptgenerator.fitting import get_fit_cmds, BASE_FIT_COMMAND, BASE_FIT_INCLUDE

EXAMPLE_FIT_PROPERTIES = {
    "EndX": 1018,
    "Function": "name=Lorentzian,Amplitude=22078.3,PeakCentre=492.226,FWHM=56.265",
    "InputWorkspace": "TestWorkspace",
    "MaxIterations": 5000,
    "Normalise": True,
    "Output": "TestWorkspaceOutput",
    "OutputCompositeMembers": True,
    "StartX": 5.6,
    "WorkspaceIndex": 4,
}

# This is an example of whats returned by the method getFitAlgorithmParameters in the FitPropertyBrowser
EXAMPLE_FIT_ALG_PROPERTIES = json.dumps({"name": "Fit", "properties": EXAMPLE_FIT_PROPERTIES, "version": 1})


class PlotScriptGeneratorFittingTest(unittest.TestCase):
    def setUp(self):
        self.mock_fig = Mock()
        self.mock_browser = Mock(spec=FitPropertyBrowser)
        self.mock_fig.canvas.manager.fit_browser = self.mock_browser

    def test_fit_commands_empty_if_no_fit(self):
        self.mock_browser.fit_result_ws_name = ""

        commands, headers = get_fit_cmds(self.mock_fig)

        self.assertEqual(commands, [])
        self.assertEqual(headers, [])

    def test_returns_empty_commands_if_no_fit_browser(self):
        mock_fig = Mock()
        # Figure manager base has no fit browser
        mock_fig.canvas.manager = Mock(spec=FigureManagerBase)

        commands, headers = get_fit_cmds(mock_fig)

        self.assertEqual(commands, [])
        self.assertEqual(headers, [])

    def test_get_fit_cmds_returns_expected_commands(self):
        self.mock_browser.fit_result_ws_name = "TestWorkspace"
        self.mock_browser.getFitAlgorithmParameters.return_value = EXAMPLE_FIT_ALG_PROPERTIES

        commands, headers = get_fit_cmds(self.mock_fig)

        self.assertEqual(headers[0], BASE_FIT_INCLUDE)
        # Should be the 9 algorithm properties + the final call to Fit
        # Check each is its expected value
        self.assertEqual(len(commands), len(EXAMPLE_FIT_PROPERTIES) + 1)
        for i in range(len(commands) - 1):
            fit_property, fit_value = tuple(commands[i].split("=", 1))
            self.assertEqual(EXAMPLE_FIT_PROPERTIES[fit_property], ast.literal_eval(fit_value))

    def test_get_fit_commands_returns_expected_fit_algorithm_call(self):
        self.mock_browser.fit_result_ws_name = "TestWorkspace"
        self.mock_browser.getFitAlgorithmParameters.return_value = EXAMPLE_FIT_ALG_PROPERTIES

        commands, headers = get_fit_cmds(self.mock_fig)
        fit_command = commands[-1]
        alg_call = re.sub(r"\([^)]*\)", "", fit_command)
        fit_args = re.findall(r"\((.*?)\)", fit_command)[0].split(",")

        # check that the alg call is the same
        # Check correct args are passed into the Fit call
        self.assertEqual(alg_call, BASE_FIT_COMMAND)
        self.assertEqual(len(fit_args), len(EXAMPLE_FIT_PROPERTIES))
        fit_arg_list = [fit_arg.split("=")[0] for fit_arg in fit_args]
        self.assertEqual(fit_arg_list, list(EXAMPLE_FIT_PROPERTIES.keys()))

    def test_get_fit_commands_returns_expected_order(self):
        self.mock_browser.fit_result_ws_name = "TestWorkspace"
        self.mock_browser.getFitAlgorithmParameters.return_value = EXAMPLE_FIT_ALG_PROPERTIES

        commands, headers = get_fit_cmds(self.mock_fig)
        fit_properties = [fit_command.split("=")[0] for fit_command in commands]

        # Test some of the properties are in the correct place
        self.assertEqual(fit_properties[0], "Function")
        self.assertEqual(fit_properties[2], "WorkspaceIndex")
        self.assertEqual(fit_properties[5], "EndX")


if __name__ == "__main__":
    unittest.main()
