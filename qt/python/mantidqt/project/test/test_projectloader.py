# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2017 ISIS Rutherford Appleton Laboratory UKRI,
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
from mantidqt.project import projectloader, projectsaver


project_file_ext = ".mtdproj"
working_directory = tempfile.mkdtemp()


class ProjectLoaderTest(unittest.TestCase):
    def setUp(self):
        ws1_name = "ws1"
        ADS.addOrReplace(ws1_name, CreateSampleWorkspace(OutputWorkspace=ws1_name))
        project_saver = projectsaver.ProjectSaver(project_file_ext)
        project_saver.save_project(workspace_to_save=[ws1_name], directory=working_directory)

    def tearDown(self):
        ADS.clear()
        if isdir(working_directory):
            rmtree(working_directory)

    def test_project_loading(self):
        project_loader = projectloader.ProjectLoader(project_file_ext)

        self.assertTrue(project_loader.load_project(working_directory))

        self.assertEqual(ADS.getObjectNames(), ["ws1"])

    def test_confirm_all_workspaces_loaded(self):
        ws1_name = "ws1"
        ADS.addOrReplace(ws1_name, CreateSampleWorkspace(OutputWorkspace=ws1_name))
        self.assertTrue(projectloader._confirm_all_workspaces_loaded(workspaces_to_confirm=[ws1_name]))


class ProjectReaderTest(unittest.TestCase):
    def setUp(self):
        ws1_name = "ws1"
        ADS.addOrReplace(ws1_name, CreateSampleWorkspace(OutputWorkspace=ws1_name))
        project_saver = projectsaver.ProjectSaver(project_file_ext)
        project_saver.save_project(workspace_to_save=[ws1_name], directory=working_directory)

    def tearDown(self):
        ADS.clear()
        if isdir(working_directory):
            rmtree(working_directory)

    def test_project_reading(self):
        project_reader = projectloader.ProjectReader(project_file_ext)
        project_reader.read_project(working_directory)
        self.assertEqual(["ws1"], project_reader.workspace_names)
        self.assertEqual({}, project_reader.interfaces_dicts)
