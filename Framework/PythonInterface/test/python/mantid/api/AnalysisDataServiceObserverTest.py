# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2017 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantidqt package
#
from __future__ import (absolute_import, division, print_function, unicode_literals)

import unittest
import sys

from mantid.api import AnalysisDataService as ADS, AnalysisDataServiceObserver
from mantid.simpleapi import CreateSampleWorkspace, RenameWorkspace, GroupWorkspaces, UnGroupWorkspace, DeleteWorkspace

if sys.version_info.major == 3:
    from unittest import mock
else:
    import mock


class FakeInheritingClass(AnalysisDataServiceObserver):
    def anyChangeHandle(self):
        # This method is going to be mocked out but needs to be present to actually get the hook from the C++ side
        pass


class AnalysisDataServiceObserverTest(unittest.TestCase):
    def setUp(self):
        self.mock_class = FakeInheritingClass()
        self.mock_class.observeAll(True)
        self.mock_class.anyChangeHandle = mock.MagicMock()

    def tearDown(self):
        self.mock_class.observeAll(False)
        ADS.clear()

    def test_observeAll_calls_anyChangeHandle_when_set_on_ads_add(self):
        CreateSampleWorkspace(OutputWorkspace="ws")
        self.assertEqual(self.mock_class.anyChangeHandle.call_count, 1)

    def test_observeAll_calls_anyChangeHandle_when_set_on_ads_replace(self):
        CreateSampleWorkspace(OutputWorkspace="ws")
        expected_count = 1

        # Will replace first workspace
        CreateSampleWorkspace(OutputWorkspace="ws")
        expected_count += 1

        self.assertEqual(self.mock_class.anyChangeHandle.call_count, expected_count)

    def test_observeAll_calls_anyChangeHandle_when_set_on_ads_delete(self):
        CreateSampleWorkspace(OutputWorkspace="ws")
        expected_count = 1

        # Will replace first workspace
        DeleteWorkspace("ws")
        expected_count += 1

        self.assertEqual(self.mock_class.anyChangeHandle.call_count, expected_count)

    def test_observeAll_calls_anyChangeHandle_when_set_on_ads_clear(self):
        CreateSampleWorkspace(OutputWorkspace="ws")
        expected_count = 1

        # Will replace first workspace
        ADS.clear()
        expected_count += 1

        self.assertEqual(self.mock_class.anyChangeHandle.call_count, expected_count)

    def test_observeAll_calls_anyChangeHandle_when_set_on_ads_rename(self):
        CreateSampleWorkspace(OutputWorkspace="ws")
        expected_count = 1

        # Will replace first workspace
        RenameWorkspace(InputWorkspace="ws", OutputWorkspace="ws1")
        # One for the rename
        expected_count += 1
        # One for replacing original named workspace
        expected_count += 1

        self.assertEqual(self.mock_class.anyChangeHandle.call_count, expected_count)

    def test_observeAll_calls_anyChangeHandle_when_set_on_ads_group_made(self):
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

        self.assertEqual(self.mock_class.anyChangeHandle.call_count, expected_count)

    def test_observeAll_calls_anyChangeHandle_when_set_on_ads_ungroup_performed(self):
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

        self.assertEqual(self.mock_class.anyChangeHandle.call_count, expected_count)

    def test_observeAll_calls_anyChangeHandle_when_set_on_ads_group_updated(self):
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

        self.assertEqual(self.mock_class.anyChangeHandle.call_count, expected_count)


if __name__ == "__main__":
    unittest.main()
