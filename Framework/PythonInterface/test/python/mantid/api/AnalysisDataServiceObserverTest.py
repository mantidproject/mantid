# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantidqt package
#
from __future__ import (absolute_import, division, print_function, unicode_literals)

import unittest

from mantid.api import AnalysisDataService as ADS, AnalysisDataServiceObserver
from mantid.simpleapi import CreateSampleWorkspace, RenameWorkspace, GroupWorkspaces, UnGroupWorkspace, DeleteWorkspace
from mantid.py3compat import mock


class FakeADSObserver(AnalysisDataServiceObserver):
    # These methods are going to be mocked out but needs to be present to actually get the hook from the C++ side
    def anyChangeHandle(self):
        pass

    def addHandle(self, wsName, ws):
        pass

    def replaceHandle(self, wsName, ws):
        pass

    def deleteHandle(self, wsName, ws):
        pass

    def clearHandle(self):
        pass

    def renameHandle(self, wsName, newName):
        pass

    def groupHandle(self, wsName, ws):
        pass

    def unGroupHandle(self, wsName, ws):
        pass

    def groupUpdateHandle(self, wsName, ws):
        pass


class AnalysisDataServiceObserverTest(unittest.TestCase):
    def setUp(self):
        self.fake_class = FakeADSObserver()
        self.fake_class.anyChangeHandle = mock.MagicMock()

    def tearDown(self):
        self.fake_class.observeAll(False)
        ADS.clear()

    def test_observeAll_calls_anyChangeHandle_when_set_on_ads_add(self):
        self.fake_class.observeAll(True)
        CreateSampleWorkspace(OutputWorkspace="ws")
        self.assertEqual(self.fake_class.anyChangeHandle.call_count, 1)

    def test_observeAll_calls_anyChangeHandle_when_set_on_ads_replace(self):
        self.fake_class.observeAll(True)
        CreateSampleWorkspace(OutputWorkspace="ws")
        expected_count = 1

        # Will replace first workspace
        CreateSampleWorkspace(OutputWorkspace="ws")
        expected_count += 1

        self.assertEqual(self.fake_class.anyChangeHandle.call_count, expected_count)

    def test_observeAll_calls_anyChangeHandle_when_set_on_ads_delete(self):
        self.fake_class.observeAll(True)
        CreateSampleWorkspace(OutputWorkspace="ws")
        expected_count = 1

        # Will replace first workspace
        DeleteWorkspace("ws")
        expected_count += 1

        self.assertEqual(self.fake_class.anyChangeHandle.call_count, expected_count)

    def test_observeAll_calls_anyChangeHandle_when_set_on_ads_clear(self):
        self.fake_class.observeAll(True)
        CreateSampleWorkspace(OutputWorkspace="ws")
        expected_count = 1

        # Will replace first workspace
        ADS.clear()
        expected_count += 1

        self.assertEqual(self.fake_class.anyChangeHandle.call_count, expected_count)

    def test_observeAll_calls_anyChangeHandle_when_set_on_ads_rename(self):
        self.fake_class.observeAll(True)
        CreateSampleWorkspace(OutputWorkspace="ws")
        expected_count = 1

        # Will replace first workspace
        RenameWorkspace(InputWorkspace="ws", OutputWorkspace="ws1")
        # One for the rename
        expected_count += 1
        # One for replacing original named workspace
        expected_count += 1

        self.assertEqual(self.fake_class.anyChangeHandle.call_count, expected_count)

    def test_observeAll_calls_anyChangeHandle_when_set_on_ads_group_made(self):
        self.fake_class.observeAll(True)
        CreateSampleWorkspace(OutputWorkspace="ws1")
        expected_count = 1
        CreateSampleWorkspace(OutputWorkspace="ws2")
        expected_count += 1

        # Will replace first workspace
        GroupWorkspaces(InputWorkspaces="ws1,ws2", OutputWorkspace="NewGroup")
        # One for grouping the workspace
        expected_count += 1
        # One for adding it to the ADS
        expected_count += 1

        self.assertEqual(self.fake_class.anyChangeHandle.call_count, expected_count)

    def test_observeAll_calls_anyChangeHandle_when_set_on_ads_ungroup_performed(self):
        self.fake_class.observeAll(True)
        CreateSampleWorkspace(OutputWorkspace="ws1")
        expected_count = 1
        CreateSampleWorkspace(OutputWorkspace="ws2")
        expected_count += 1
        GroupWorkspaces(InputWorkspaces="ws1,ws2", OutputWorkspace="NewGroup")
        # One for grouping the workspace
        expected_count += 1
        # One for adding it to the ADS
        expected_count += 1

        # Will replace first workspace
        UnGroupWorkspace(InputWorkspace="NewGroup")
        # One for ungrouping the workspace
        expected_count += 1
        # One for removing the grouped workspace object from the ADS
        expected_count += 1

        self.assertEqual(self.fake_class.anyChangeHandle.call_count, expected_count)

    def test_observeAll_calls_anyChangeHandle_when_set_on_ads_group_updated(self):
        self.fake_class.observeAll(True)
        CreateSampleWorkspace(OutputWorkspace="ws1")
        expected_count = 1
        CreateSampleWorkspace(OutputWorkspace="ws2")
        expected_count += 1
        CreateSampleWorkspace(OutputWorkspace="ws3")
        expected_count += 1
        GroupWorkspaces(InputWorkspaces="ws1,ws2", OutputWorkspace="NewGroup")
        # One for grouping the workspace
        expected_count += 1
        # One for adding it to the ADS
        expected_count += 1

        # Will update group
        ADS.addToGroup("NewGroup", "ws3")
        expected_count += 1

        self.assertEqual(self.fake_class.anyChangeHandle.call_count, expected_count)

    def test_observeAdd_calls_addHandle_when_set_on_ads_and_a_workspace_is_added(self):
        self.fake_class.observeAdd(True)
        self.fake_class.addHandle = mock.MagicMock()
        CreateSampleWorkspace(OutputWorkspace="ws")
        self.assertEqual(self.fake_class.addHandle.call_count, 1)

    def test_observeReplace_calls_replaceHandle_when_set_on_ads_and_a_workspace_is_replaced(self):
        CreateSampleWorkspace(OutputWorkspace="ws")

        self.fake_class.observeReplace(True)
        self.fake_class.replaceHandle = mock.MagicMock()
        CreateSampleWorkspace(OutputWorkspace="ws")

        self.assertEqual(self.fake_class.replaceHandle.call_count, 1)

    def test_observeDelete_calls_deleteHandle_when_set_on_ads_and_a_workspace_is_deleted(self):
        CreateSampleWorkspace(OutputWorkspace="ws")

        self.fake_class.observeDelete(True)
        self.fake_class.deleteHandle = mock.MagicMock()
        ADS.remove("ws")

        self.assertEqual(self.fake_class.deleteHandle.call_count, 1)

    def test_observeClear_calls_clearHandle_when_set_on_ads_its_cleared(self):
        CreateSampleWorkspace(OutputWorkspace="ws")

        self.fake_class.observeClear(True)
        self.fake_class.clearHandle = mock.MagicMock()
        ADS.clear()

        self.assertEqual(self.fake_class.clearHandle.call_count, 1)

    def test_observeRename_calls_renameHandle_when_set_on_ads_and_a_workspace_is_renamed(self):
        CreateSampleWorkspace(OutputWorkspace="ws")

        self.fake_class.observeRename(True)
        self.fake_class.renameHandle = mock.MagicMock()
        RenameWorkspace(InputWorkspace="ws", OutputWorkspace="ws1")

        self.assertEqual(self.fake_class.renameHandle.call_count, 1)

    def test_observeGroup_calls_groupHandle_when_set_on_ads_and_a_group_workspace_is_made(self):
        CreateSampleWorkspace(OutputWorkspace="ws1")
        CreateSampleWorkspace(OutputWorkspace="ws2")

        self.fake_class.observeGroup(True)
        self.fake_class.groupHandle = mock.MagicMock()
        GroupWorkspaces(InputWorkspaces="ws1,ws2", OutputWorkspace="NewGroup")

        self.assertEqual(self.fake_class.groupHandle.call_count, 1)

    def test_observeUnGroup_calls_unGroupHandle_when_set_on_ads_and_a_group_is_ungrouped(self):
        CreateSampleWorkspace(OutputWorkspace="ws1")
        CreateSampleWorkspace(OutputWorkspace="ws2")
        GroupWorkspaces(InputWorkspaces="ws1,ws2", OutputWorkspace="NewGroup")

        self.fake_class.observeUnGroup(True)
        self.fake_class.unGroupHandle = mock.MagicMock()
        UnGroupWorkspace(InputWorkspace="NewGroup")

        self.assertEqual(self.fake_class.unGroupHandle.call_count, 1)

    def test_observeGroupUpdated_calls_groupUpdateHandle_when_set_on_ads_and_a_group_in_the_ads_is_updated(self):
        CreateSampleWorkspace(OutputWorkspace="ws1")
        CreateSampleWorkspace(OutputWorkspace="ws2")
        CreateSampleWorkspace(OutputWorkspace="ws3")
        GroupWorkspaces(InputWorkspaces="ws1,ws2", OutputWorkspace="NewGroup")

        self.fake_class.observeGroupUpdate(True)
        self.fake_class.groupUpdateHandle = mock.MagicMock()
        ADS.addToGroup("NewGroup", "ws3")

        self.assertEqual(self.fake_class.groupUpdateHandle.call_count, 1)


if __name__ == "__main__":
    unittest.main()
