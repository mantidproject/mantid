# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantidqt package
#

import unittest

from os.path import isdir
from shutil import rmtree
import tempfile

from mantid.api import AnalysisDataService as ADS
from mantid.simpleapi import CreateSampleWorkspace
from mantidqt.project import projectsaver, workspaceloader


class WorkspaceLoaderTest(unittest.TestCase):
    def setUp(self):
        self.working_directory = tempfile.mkdtemp()
        self.ws1_name = "ws1"
        self.project_ext = ".mtdproj"
        ADS.addOrReplace(self.ws1_name, CreateSampleWorkspace(OutputWorkspace=self.ws1_name))
        project_saver = projectsaver.ProjectSaver(self.project_ext)
        project_saver.save_project(workspace_to_save=[self.ws1_name], file_name=self.working_directory)

    def tearDown(self):
        ADS.clear()
        if isdir(self.working_directory):
            rmtree(self.working_directory)

    def test_workspace_loading(self):
        workspace_loader = workspaceloader.WorkspaceLoader()
        workspace_loader.load_workspaces(self.working_directory, workspaces_to_load=[self.ws1_name])
        self.assertEqual(ADS.getObjectNames(), [self.ws1_name])


if __name__ == "__main__":
    unittest.main()
