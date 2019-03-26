# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantidqt package
#
import json
import matplotlib.backend_bases
import matplotlib.figure
import os
import tempfile
import unittest
from shutil import rmtree

from mantid.api import AnalysisDataService as ADS
from mantid.simpleapi import CreateSampleWorkspace
from mantid.py3compat import mock
from mantidqt.project import projectsaver


project_file_ext = ".mtdproj"
working_directory = tempfile.mkdtemp()
working_project_file = os.path.join(working_directory, "temp" + project_file_ext)


class ProjectSaverTest(unittest.TestCase):
    def tearDown(self):
        ADS.clear()

    def setUp(self):
        # In case it was hard killed and is still present
        if os.path.isdir(working_directory):
            rmtree(working_directory)

    def test_only_one_workspace_saving(self):
        ws1_name = "ws1"
        ADS.addOrReplace(ws1_name, CreateSampleWorkspace(OutputWorkspace=ws1_name))
        project_saver = projectsaver.ProjectSaver(project_file_ext)

        workspaces_string = "\"workspaces\": [\"ws1\"]"
        plots_string = "\"plots\": []"

        project_saver.save_project(workspace_to_save=[ws1_name], file_name=working_project_file)

        # Check project file is saved correctly
        f = open(working_project_file, "r")
        file_string = f.read()
        self.assertTrue(workspaces_string in file_string)
        self.assertTrue(plots_string in file_string)

        # Check workspace is saved
        list_of_files = os.listdir(working_directory)
        self.assertEqual(len(list_of_files), 2)
        self.assertTrue(os.path.basename(working_project_file) in list_of_files)
        self.assertTrue(ws1_name + ".nxs" in list_of_files)

    def test_only_multiple_workspaces_saving(self):
        ws1_name = "ws1"
        ws2_name = "ws2"
        ws3_name = "ws3"
        ws4_name = "ws4"
        ws5_name = "ws5"
        CreateSampleWorkspace(OutputWorkspace=ws1_name)
        CreateSampleWorkspace(OutputWorkspace=ws2_name)
        CreateSampleWorkspace(OutputWorkspace=ws3_name)
        CreateSampleWorkspace(OutputWorkspace=ws4_name)
        CreateSampleWorkspace(OutputWorkspace=ws5_name)
        project_saver = projectsaver.ProjectSaver(project_file_ext)

        workspaces_string = "\"workspaces\": [\"ws1\", \"ws2\", \"ws3\", \"ws4\", \"ws5\"]"
        plots_string = "\"plots\": []"

        project_saver.save_project(workspace_to_save=[ws1_name, ws2_name, ws3_name, ws4_name, ws5_name],
                                   file_name=working_project_file)

        # Check project file is saved correctly
        f = open(working_project_file, "r")
        file_string = f.read()
        self.assertTrue(workspaces_string in file_string)
        self.assertTrue(plots_string in file_string)

        # Check workspace is saved
        list_of_files = os.listdir(working_directory)
        self.assertEqual(len(list_of_files), 6)
        self.assertTrue(os.path.basename(working_project_file) in list_of_files)
        self.assertTrue(ws1_name + ".nxs" in list_of_files)
        self.assertTrue(ws2_name + ".nxs" in list_of_files)
        self.assertTrue(ws3_name + ".nxs" in list_of_files)
        self.assertTrue(ws4_name + ".nxs" in list_of_files)
        self.assertTrue(ws5_name + ".nxs" in list_of_files)

    def test_only_saving_one_workspace_when_multiple_are_present_in_the_ADS(self):
        ws1_name = "ws1"
        ws2_name = "ws2"
        ws3_name = "ws3"
        CreateSampleWorkspace(OutputWorkspace=ws1_name)
        CreateSampleWorkspace(OutputWorkspace=ws2_name)
        CreateSampleWorkspace(OutputWorkspace=ws3_name)
        project_saver = projectsaver.ProjectSaver(project_file_ext)

        workspaces_string = "\"workspaces\": [\"ws1\"]"
        plots_string = "\"plots\": []"

        project_saver.save_project(workspace_to_save=[ws1_name], file_name=working_project_file)

        # Check project file is saved correctly
        f = open(working_project_file, "r")
        file_string = f.read()
        self.assertTrue(workspaces_string in file_string)
        self.assertTrue(plots_string in file_string)

        # Check workspace is saved
        list_of_files = os.listdir(working_directory)
        self.assertEqual(len(list_of_files), 2)
        self.assertTrue(os.path.basename(working_project_file) in list_of_files)
        self.assertTrue(ws1_name + ".nxs" in list_of_files)

    def test_saving_plots_when_plots_are_passed(self):
        os.makedirs(working_directory)
        fig = matplotlib.figure.Figure(dpi=100, figsize=(6.4, 4.8))
        fig_manager = matplotlib.backend_bases.FigureManagerBase(matplotlib.backend_bases.FigureCanvasBase(fig), 1)
        matplotlib.axes.Axes(fig=fig, rect=[0, 0, 0, 0])

        project_saver = projectsaver.ProjectSaver(project_file_ext)

        project_saver.save_project(file_name=working_project_file,
                                   plots_to_save={1: fig_manager})

        plots_dict = {u"creationArguments": [], u"axes": [], u"label": u"", u"properties": {u"figWidth": 6.4,
                                                                                            u"figHeight": 4.8,
                                                                                            u"dpi": 100.0}}

        f = open(working_project_file, "r")
        file_dict = json.load(f)
        self.assertDictEqual(plots_dict, file_dict["plots"][0])

    @mock.patch('mantidqt.project.projectsaver.ProjectWriter')
    def test_save_workspaces_path_when_false(self, pwriter):
        CreateSampleWorkspace(OutputWorkspace='ws1')
        file_ext = '.recfile'
        saver = projectsaver.ProjectSaver(file_ext)

        saver.save_project(file_name=working_project_file, project_recovery=False)

        self.assertEqual(pwriter.call_args, mock.call(interfaces_to_save=[], plots_to_save=[],
                                                      project_file_ext=file_ext, save_location=working_project_file,
                                                      workspace_names=['ws1']))


class ProjectWriterTest(unittest.TestCase):
    def tearDown(self):
        ADS.clear()

    def setUp(self):
        # In case it was hard killed and is still present
        if os.path.isdir(working_directory):
            rmtree(working_directory)

        os.makedirs(working_directory)

    def test_write_out_empty_workspaces(self):
        workspace_list = []
        plots_to_save = []
        interfaces_to_save = []
        project_writer = projectsaver.ProjectWriter(save_location=working_project_file, workspace_names=workspace_list,
                                                    project_file_ext=project_file_ext, plots_to_save=plots_to_save,
                                                    interfaces_to_save=interfaces_to_save)
        workspaces_string = "\"workspaces\": []"
        plots_string = "\"plots\": []"

        project_writer.write_out()

        with open(working_project_file, 'r') as f:
            file_string = f.read()
        self.assertTrue(workspaces_string in file_string)
        self.assertTrue(plots_string in file_string)

    def test_write_out_on_just_workspaces(self):
        plots_to_save = []
        workspace_list = ["ws1", "ws2", "ws3", "ws4"]
        interfaces_to_save = []
        project_writer = projectsaver.ProjectWriter(save_location=working_project_file, workspace_names=workspace_list,
                                                    project_file_ext=project_file_ext, plots_to_save=plots_to_save,
                                                    interfaces_to_save=interfaces_to_save)
        workspaces_string = "\"workspaces\": [\"ws1\", \"ws2\", \"ws3\", \"ws4\"]"
        plots_string = "\"plots\": []"

        project_writer.write_out()
        with open(working_project_file, 'r') as f:
            file_string = f.read()
        self.assertTrue(workspaces_string in file_string)
        self.assertTrue(plots_string in file_string)

    def test_write_out_on_just_plots(self):
        plots_to_save = [{"plots1": {"plot-information": "axes data"}}]
        workspace_list = []
        interfaces_to_save = []
        project_writer = projectsaver.ProjectWriter(save_location=working_project_file, workspace_names=workspace_list,
                                                    project_file_ext=project_file_ext, plots_to_save=plots_to_save,
                                                    interfaces_to_save=interfaces_to_save)
        workspaces_string = "\"workspaces\": []"
        plots_string = "\"plots\": [{\"plots1\": {\"plot-information\": \"axes data\"}}]"

        project_writer.write_out()

        with open(working_project_file, 'r') as f:
            file_string = f.read()
        self.assertTrue(workspaces_string in file_string)
        self.assertTrue(plots_string in file_string)

    def test_write_out_on_both_workspaces_and_plots(self):
        plots_to_save = [{"plots1": {"plot-information": "axes data"}}]
        workspace_list = ["ws1", "ws2", "ws3", "ws4"]
        interfaces_to_save = []
        project_writer = projectsaver.ProjectWriter(save_location=working_project_file, workspace_names=workspace_list,
                                                    project_file_ext=project_file_ext, plots_to_save=plots_to_save,
                                                    interfaces_to_save=interfaces_to_save)
        workspaces_string = "\"workspaces\": [\"ws1\", \"ws2\", \"ws3\", \"ws4\"]"
        plots_string = "\"plots\": [{\"plots1\": {\"plot-information\": \"axes data\"}}]"

        project_writer.write_out()

        with open(working_project_file, 'r') as f:
            file_string = f.read()
        self.assertTrue(workspaces_string in file_string)
        self.assertTrue(plots_string in file_string)

    def test_write_out_on_interfaces(self):
        plots_to_save = []
        workspace_list = []
        interfaces_to_save = [{"interface1": {"interface data": "data"}}]
        project_writer = projectsaver.ProjectWriter(save_location=working_project_file, workspace_names=workspace_list,
                                                    project_file_ext=project_file_ext, plots_to_save=plots_to_save,
                                                    interfaces_to_save=interfaces_to_save)
        workspaces_string = "\"workspaces\": []"
        plots_string = "\"plots\": []"
        interface_string = "\"interfaces\": [{\"interface1\": {\"interface data\": \"data\"}}]"

        project_writer.write_out()

        with open(working_project_file, 'r') as f:
            file_string = f.read()
        self.assertTrue(workspaces_string in file_string)
        self.assertTrue(plots_string in file_string)
        self.assertTrue(interface_string in file_string)


if __name__ == "__main__":
    unittest.main()
