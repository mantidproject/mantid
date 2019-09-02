# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantidqt package
#

import unittest

from os import listdir
from os.path import isdir
from shutil import rmtree
import tempfile
import mock

from mantid.api import AnalysisDataService as ADS, IMDEventWorkspace  # noqa
from mantid.dataobjects import MDHistoWorkspace, MaskWorkspace  # noqa
from mantidqt.project import workspacesaver
from mantid.simpleapi import (CreateSampleWorkspace, CreateMDHistoWorkspace, LoadMD, LoadMask, MaskDetectors,  # noqa
                              ExtractMask, GroupWorkspaces)  # noqa


class WorkspaceSaverTest(unittest.TestCase):
    def setUp(self):
        self.working_directory = tempfile.mkdtemp()

    def tearDown(self):
        ADS.clear()
        if isdir(self.working_directory):
            rmtree(self.working_directory)

    def test_saving_single_workspace(self):
        ws_saver = workspacesaver.WorkspaceSaver(self.working_directory)
        ws1 = CreateSampleWorkspace()
        ws1_name = "ws1"

        ADS.addOrReplace(ws1_name, ws1)
        ws_saver.save_workspaces([ws1_name])

        list_of_files = listdir(self.working_directory)
        self.assertEqual(len(list_of_files), 1)
        self.assertTrue(ws1_name + ".nxs" in list_of_files)

    def test_saving_multiple_workspaces(self):
        ws_saver = workspacesaver.WorkspaceSaver(self.working_directory)
        ws1 = CreateSampleWorkspace()
        ws1_name = "ws1"
        ws2 = CreateSampleWorkspace()
        ws2_name = "ws2"

        ADS.addOrReplace(ws1_name, ws1)
        ADS.addOrReplace(ws2_name, ws2)
        ws_saver.save_workspaces([ws1_name, ws2_name])

        list_of_files = listdir(self.working_directory)
        self.assertEqual(len(list_of_files), 2)
        self.assertTrue(ws2_name + ".nxs" in list_of_files)
        self.assertTrue(ws1_name + ".nxs" in list_of_files)

    def test_when_MDWorkspace_is_in_ADS(self):
        ws_saver = workspacesaver.WorkspaceSaver(self.working_directory)
        ws1 = CreateMDHistoWorkspace(SignalInput='1,2,3,4,5,6,7,8,9', ErrorInput='1,1,1,1,1,1,1,1,1',
                                     Dimensionality='2', Extents='-1,1,-1,1', NumberOfBins='3,3', Names='A,B',
                                     Units='U,T')
        ws1_name = "ws1"

        ADS.addOrReplace(ws1_name, ws1)
        ws_saver.save_workspaces([ws1_name])

        list_of_files = listdir(self.working_directory)
        self.assertEqual(len(list_of_files), 1)
        self.assertTrue(ws1_name + ".nxs" in list_of_files)
        self._load_MDWorkspace_and_test_it(ws1_name)

    @mock.patch("mantidqt.project.workspacesaver.logger")
    def test_when_nested_workspaces_are_being_saved_from_the_ADS(self, logger):
        CreateSampleWorkspace(OutputWorkspace="ws1")
        CreateSampleWorkspace(OutputWorkspace="ws2")
        CreateSampleWorkspace(OutputWorkspace="ws3")
        CreateSampleWorkspace(OutputWorkspace="ws4")
        GroupWorkspaces(InputWorkspaces="ws1,ws2", OutputWorkspace="group1")
        GroupWorkspaces(InputWorkspaces="ws4,ws3", OutputWorkspace="group2")
        ADS.addToGroup("group2", "group1")
        ws_saver = workspacesaver.WorkspaceSaver(self.working_directory)

        ws_saver.save_workspaces(["group2"])

        self.assertListEqual(["group1", "group2", "ws1", "ws2", "ws3", "ws4"], ADS.getObjectNames())
        logger.warning.assert_called_with(u'Couldn\'t save workspace in project: "group2" because SaveNexusProcessed: '
                                          u'NeXus files do not support nested groups of groups')

    def _load_MDWorkspace_and_test_it(self, save_name):
        filename = self.working_directory + '/' + save_name + ".nxs"
        ws = LoadMD(Filename=filename)
        ws_is_a_mdworkspace = isinstance(ws, IMDEventWorkspace) or isinstance(ws, MDHistoWorkspace)
        self.assertEqual(ws_is_a_mdworkspace, True)


if __name__ == "__main__":
    unittest.main()
