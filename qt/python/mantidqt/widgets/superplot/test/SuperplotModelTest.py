# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +

import unittest
from unittest import mock

from mantid.api import WorkspaceGroup

from mantidqt.widgets.superplot.SuperplotModel import SuperplotModel


class SuperplotModelTest(unittest.TestCase):

    def setUp(self):
        pyModule = "mantidqt.widgets.superplot.SuperplotModel"

        patch = mock.patch(pyModule + ".SuperplotAdsObserver")
        self.mObs = patch.start()
        self.mObs = self.mObs.return_value
        self.addCleanup(patch.stop)

        patch = mock.patch(pyModule + ".mtd")
        self.mMtd = patch.start()
        self.addCleanup(patch.stop)

        self.model = SuperplotModel()

    def test_init(self):
        self.assertEqual(self.model._workspaces, [])
        self.assertEqual(self.model._plottedData, [])
        self.assertIsNone(self.model._plotMode)
        self.mObs.signals.wsDeleted.connect.assert_called_once()
        self.mObs.signals.wsRenamed.connect.assert_called_once()
        self.mObs.signals.wsReplaced.connect.assert_called_once()

    def test_addWorkspace(self):
        self.mMtd.__contains__.return_value = False
        self.model.addWorkspace("ws1")
        self.assertEqual(self.model._workspaces, [])
        self.mMtd.__contains__.return_value = True
        self.model.addWorkspace("ws1")
        self.assertEqual(self.model._workspaces, ["ws1"])
        self.model.addWorkspace("ws1")
        self.assertEqual(self.model._workspaces, ["ws1"])
        self.model.addWorkspace("ws2")
        self.assertEqual(self.model._workspaces, ["ws1", "ws2"])
        self.mMtd.__getitem__.return_value = mock.Mock(spec=WorkspaceGroup)
        self.mMtd.__getitem__.return_value.getNames.return_value = ["ws2",
                                                                    "ws3",
                                                                    "ws4"]
        self.model.addWorkspace("g1")
        self.assertEqual(self.model._workspaces, ["ws1", "ws2", "ws3", "ws4"])

    def test_delWorkspace(self):
        self.model._workspaces = ["ws1", "ws2", "ws3"]
        self.model._plottedData = [("ws1", 1), ("ws2", 2)]
        self.model._plotMode = self.model.BIN_MODE
        self.model.delWorkspace("ws4")
        self.assertEqual(self.model._workspaces, ["ws1", "ws2", "ws3"])
        self.assertEqual(self.model._plottedData, [("ws1", 1), ("ws2", 2)])
        self.assertEqual(self.model._plotMode, self.model.BIN_MODE)
        self.model.delWorkspace("ws3")
        self.assertEqual(self.model._workspaces, ["ws1", "ws2"])
        self.assertEqual(self.model._plottedData, [("ws1", 1), ("ws2", 2)])
        self.assertEqual(self.model._plotMode, self.model.BIN_MODE)
        self.model.delWorkspace("ws2")
        self.assertEqual(self.model._workspaces, ["ws1"])
        self.assertEqual(self.model._plottedData, [("ws1", 1)])
        self.assertEqual(self.model._plotMode, self.model.BIN_MODE)
        self.model.delWorkspace("ws1")
        self.assertEqual(self.model._workspaces, [])
        self.assertEqual(self.model._plottedData, [])
        self.assertIsNone(self.model._plotMode)

    def test_getWorkspaces(self):
        self.model._workspaces = ["ws1", "ws2", "ws3"]
        wsList = self.model.getWorkspaces()
        self.assertEqual(wsList, self.model._workspaces)
        # test that getWorkspace is returning a copy
        wsList = wsList[0:2]
        self.assertEqual(wsList, ["ws1", "ws2"])
        self.assertEqual(self.model._workspaces, ["ws1", "ws2", "ws3"])

    def test_setBinMode(self):
        self.assertIsNone(self.model._plotMode)
        self.model.setBinMode()
        self.assertEqual(self.model._plotMode, self.model.BIN_MODE)
        self.model._plotMode = self.model.SPECTRUM_MODE
        self.model.setBinMode()
        self.assertEqual(self.model._plotMode, self.model.BIN_MODE)

    def test_setSpectrumMode(self):
        self.assertIsNone(self.model._plotMode)
        self.model.setSpectrumMode()
        self.assertEqual(self.model._plotMode, self.model.SPECTRUM_MODE)
        self.model._plotMode = self.model.BIN_MODE
        self.model.setSpectrumMode()
        self.assertEqual(self.model._plotMode, self.model.SPECTRUM_MODE)

    def test_isBinMode(self):
        self.assertIsNone(self.model._plotMode)
        self.assertFalse(self.model.isBinMode())
        self.model._plotMode = self.model.BIN_MODE
        self.assertTrue(self.model.isBinMode())
        self.model._plotMode = self.model.SPECTRUM_MODE
        self.assertFalse(self.model.isBinMode())

    def test_isSpectrumMode(self):
        self.assertIsNone(self.model._plotMode)
        self.assertFalse(self.model.isSpectrumMode())
        self.model._plotMode = self.model.SPECTRUM_MODE
        self.assertTrue(self.model.isSpectrumMode())
        self.model._plotMode = self.model.BIN_MODE
        self.assertFalse(self.model.isSpectrumMode())

    def test_addData(self):
        self.assertEqual(self.model._plottedData, [])
        self.model.addData("ws1", 1)
        self.assertEqual(self.model._plottedData, [("ws1", 1)])
        self.model.addData("ws1", 2)
        self.assertEqual(self.model._plottedData, [("ws1", 1), ("ws1", 2)])
        self.model.addData("ws1", 1)
        self.assertEqual(self.model._plottedData, [("ws1", 1), ("ws1", 2)])

    def test_removeData(self):
        self.assertEqual(self.model._plottedData, [])
        self.model.removeData("ws1", 1)
        self.assertEqual(self.model._plottedData, [])
        self.model._plottedData = [("ws1", 1), ("ws1", 2), ("ws2", 1)]
        self.model._plotMode = self.model.BIN_MODE
        self.model.removeData("ws1", 1)
        self.assertEqual(self.model._plottedData, [("ws1", 2), ("ws2", 1)])
        self.assertEqual(self.model._plotMode, self.model.BIN_MODE)
        self.model.removeData("ws2", 1)
        self.assertEqual(self.model._plottedData, [("ws1", 2)])
        self.assertEqual(self.model._plotMode, self.model.BIN_MODE)
        self.model.removeData("ws1", 2)
        self.assertEqual(self.model._plottedData, [])
        self.assertIsNone(self.model._plotMode)

    def test_getPlottedData(self):
        self.assertEqual(self.model._plottedData, [])
        self.assertEqual(self.model.getPlottedData(), [])
        self.model._plottedData = [("ws1", 1), ("ws1", 2), ("ws2", 1)]
        data = self.model.getPlottedData()
        self.assertEqual(data, [("ws1", 1), ("ws1", 2), ("ws2", 1)])
        data = data[0:2]
        self.assertEqual(data, [("ws1", 1), ("ws1", 2)])
        self.assertEqual(self.model._plottedData,
                         [("ws1", 1), ("ws1", 2), ("ws2", 1)])

    def test_onWorkspaceDeleted(self):
        self.model._workspaces = ["ws1", "ws2", "ws3"]
        self.model._plottedData = [("ws1", 1), ("ws1", 3), ("ws3", 10)]
        self.model._plotMode = self.model.SPECTRUM_MODE
        self.model.workspaceDeleted = mock.Mock()
        self.model.onWorkspaceDeleted("ws4")
        self.assertEqual(self.model._workspaces, ["ws1", "ws2", "ws3"])
        self.assertEqual(self.model._plottedData,
                         [("ws1", 1), ("ws1", 3), ("ws3", 10)])
        self.assertEqual(self.model._plotMode, self.model.SPECTRUM_MODE)
        self.model.workspaceDeleted.emit.assert_not_called()
        self.model.onWorkspaceDeleted("ws1")
        self.assertEqual(self.model._workspaces, ["ws2", "ws3"])
        self.assertEqual(self.model._plottedData, [("ws3", 10)])
        self.assertEqual(self.model._plotMode, self.model.SPECTRUM_MODE)
        self.model.workspaceDeleted.emit.assert_called_once_with("ws1")
        self.model.workspaceDeleted.reset_mock()
        self.model.onWorkspaceDeleted("ws2")
        self.assertEqual(self.model._workspaces, ["ws3"])
        self.assertEqual(self.model._plottedData, [("ws3", 10)])
        self.assertEqual(self.model._plotMode, self.model.SPECTRUM_MODE)
        self.model.workspaceDeleted.emit.assert_called_once_with("ws2")
        self.model.workspaceDeleted.reset_mock()
        self.model.onWorkspaceDeleted("ws3")
        self.assertEqual(self.model._workspaces, [])
        self.assertEqual(self.model._plottedData, [])
        self.assertIsNone(self.model._plotMode)
        self.model.workspaceDeleted.emit.assert_called_once_with("ws3")
        self.model.workspaceDeleted.reset_mock()

    def test_onWorkspaceRenamed(self):
        self.model._workspaces = ["ws1", "ws2", "ws3"]
        self.model._plottedData = [("ws1", 1), ("ws1", 3), ("ws3", 10)]
        self.model.workspaceRenamed = mock.Mock()
        self.model.onWorkspaceRenamed("ws4", "ws5")
        self.assertEqual(self.model._workspaces, ["ws1", "ws2", "ws3"])
        self.assertEqual(self.model._plottedData,
                         [("ws1", 1), ("ws1", 3), ("ws3", 10)])
        self.model.workspaceRenamed.emit.assert_not_called()
        self.model.onWorkspaceRenamed("ws1", "ws5")
        self.assertEqual(self.model._workspaces, ["ws5", "ws2", "ws3"])
        self.assertEqual(self.model._plottedData,
                         [("ws5", 1), ("ws5", 3), ("ws3", 10)])
        self.model.workspaceRenamed.emit.assert_called_once_with("ws1", "ws5")

    def test_onWorkspaceReplaced(self):
        self.model._workspaces = ["ws1", "ws2", "ws3"]
        self.model.workspaceReplaced = mock.Mock()
        self.model.onWorkspaceReplaced("ws4")
        self.model.workspaceReplaced.emit.assert_not_called()
        self.model.onWorkspaceReplaced("ws1")
        self.model.workspaceReplaced.emit.assert_called_once_with("ws1")
