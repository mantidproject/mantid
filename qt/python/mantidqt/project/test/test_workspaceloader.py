# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2017 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantidqt package
#

import unittest

from os.path import isdir, expanduser
from shutil import rmtree

from mantid.api import AnalysisDataService as ADS
from mantid.simpleapi import CreateSampleWorkspace
from mantidqt.project import projectsaver, workspaceloader


working_directory = expanduser("~") + "/workspace_loader_test"


class WorkspaceLoaderTest(unittest.TestCase):
    def setUp(self):
        ws1_name = "ws1"
        ADS.addOrReplace(ws1_name, CreateSampleWorkspace(OutputWorkspace=ws1_name))
        project_saver = projectsaver.ProjectSaver()
        project_saver.save_project(workspace_to_save=[ws1_name], directory=working_directory)

    def tearDown(self):
        ADS.clear()
        if isdir(working_directory):
            rmtree(working_directory)

    def test_workspace_loading(self):
        workspace_loader = workspaceloader.WorkspaceLoader()
        workspace_loader.load_workspaces(working_directory)
        self.assertEqual(ADS.getObjectNames(), ["ws1"])
