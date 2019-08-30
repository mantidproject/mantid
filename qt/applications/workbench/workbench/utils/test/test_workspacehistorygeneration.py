# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantidworkbench package

import unittest

from mantid.simpleapi import CreateSampleWorkspace, GroupWorkspaces
from mantid.api import AnalysisDataService as ADS
from workbench.utils.workspacehistorygeneration import get_workspace_history_list, guarantee_unique_lines, \
    get_all_workspace_history_from_ads, convert_list_to_string


def generate_commands(script):
    script_lines = script.split("\n")
    commands = []
    for script_line in script_lines:
        line_contents = script_line.split(" # ")
        commands.append(line_contents[0])
    return commands


class WorkspaceHistoryGenerationTest(unittest.TestCase):
    def tearDown(self):
        ADS.clear()

    def test_get_workspace_history_list(self):
        CreateSampleWorkspace(OutputWorkspace="ws")

        history_list = get_workspace_history_list("ws")

        self.assertEquals(len(history_list), 1)
        commands = generate_commands(history_list[0])
        self.assertEquals(commands[0],
                          "CreateSampleWorkspace(OutputWorkspace='ws')")

    def test_guarantee_unique_lines(self):
        CreateSampleWorkspace(OutputWorkspace="ws")
        history_list = get_workspace_history_list("ws")
        history_list.append(history_list[0])
        script = convert_list_to_string(history_list)

        script = guarantee_unique_lines(script)

        commands = generate_commands(script)
        self.assertEquals(len(commands), 1)
        self.assertEquals(commands[0], "CreateSampleWorkspace(OutputWorkspace='ws')")

    def test_get_all_workspace_history_from_ads(self):
        CreateSampleWorkspace(OutputWorkspace="ws")
        CreateSampleWorkspace(OutputWorkspace="ws1")
        GroupWorkspaces(InputWorkspaces="ws,ws1", OutputWorkspace="Group")

        script = get_all_workspace_history_from_ads()

        commands = generate_commands(script)
        self.assertEquals(len(commands), 3)
        self.assertEquals(commands[0], "CreateSampleWorkspace(OutputWorkspace='ws')")
        self.assertEquals(commands[1], "CreateSampleWorkspace(OutputWorkspace='ws1')")
        self.assertEquals(commands[2], "GroupWorkspaces(InputWorkspaces='ws,ws1', OutputWorkspace='Group')")
