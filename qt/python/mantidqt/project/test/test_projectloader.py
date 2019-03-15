# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantidqt package
#
import matplotlib
import tempfile
import unittest
from os.path import isdir
from shutil import rmtree

matplotlib.use('AGG')

from os.path import isdir, join  # noqa
from shutil import rmtree  # noqa
import tempfile  # noqa

from mantid.api import AnalysisDataService as ADS  # noqa
from mantid.simpleapi import CreateSampleWorkspace  # noqa
from mantid.py3compat import mock
from mantidqt.project import projectloader, projectsaver  # noqa


project_file_ext = ".mtdproj"
working_directory = tempfile.mkdtemp()
working_project_file = join(working_directory, "temp" + project_file_ext)


class ProjectLoaderTest(unittest.TestCase):
    def setUp(self):
        ws1_name = "ws1"
        ADS.addOrReplace(ws1_name, CreateSampleWorkspace(OutputWorkspace=ws1_name))
        project_saver = projectsaver.ProjectSaver(project_file_ext)
        project_saver.save_project(workspace_to_save=[ws1_name], file_name=working_project_file)

    def tearDown(self):
        ADS.clear()
        if isdir(working_directory):
            rmtree(working_directory)

    def test_project_loading_when_directory_is_none(self):
        # Tests that error handling of a value being none receives a None back.
        project_loader = projectloader.ProjectLoader(project_file_ext)

        self.assertEqual(project_loader.load_project(None), None)

    def test_project_loading(self):
        project_loader = projectloader.ProjectLoader(project_file_ext)

        self.assertTrue(project_loader.load_project(working_project_file))

        self.assertEqual(ADS.getObjectNames(), ["ws1"])

    def test_confirm_all_workspaces_loaded(self):
        ws1_name = "ws1"
        ADS.addOrReplace(ws1_name, CreateSampleWorkspace(OutputWorkspace=ws1_name))
        self.assertTrue(projectloader._confirm_all_workspaces_loaded(workspaces_to_confirm=[ws1_name]))

    def test_workspace_loader_load_workspaces_not_called_when_load_workspaces_is_false(self):
        loader = projectloader.ProjectLoader('.recfile')

        loader.workspace_loader = mock.MagicMock()

        loader.load_project(working_directory, load_workspaces=False)
        self.assertEqual(0, loader.workspace_loader.load_workspaces.call_count)


class ProjectReaderTest(unittest.TestCase):
    def setUp(self):
        ws1_name = "ws1"
        ADS.addOrReplace(ws1_name, CreateSampleWorkspace(OutputWorkspace=ws1_name))
        project_saver = projectsaver.ProjectSaver(project_file_ext)
        project_saver.save_project(workspace_to_save=[ws1_name], file_name=working_project_file)

    def tearDown(self):
        ADS.clear()
        if isdir(working_directory):
            rmtree(working_directory)

    def test_project_reading(self):
        project_reader = projectloader.ProjectReader(project_file_ext)
        project_reader.read_project(working_project_file)
        self.assertEqual(["ws1"], project_reader.workspace_names)


if __name__ == "__main__":
    unittest.main()
