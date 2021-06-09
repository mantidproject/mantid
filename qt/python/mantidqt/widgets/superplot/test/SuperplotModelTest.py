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
        self.mObs.signals.sig_ws_deleted.connect.assert_called_once()
        self.mObs.signals.sig_ws_renamed.connect.assert_called_once()
        self.mObs.signals.sig_ws_replaced.connect.assert_called_once()

    def test_addWorkspace(self):
        self.mMtd.__contains__.return_value = False
        self.model.add_workspace("ws1")
        self.assertEqual(self.model._workspaces, [])
        self.mMtd.__contains__.return_value = True
        self.model.add_workspace("ws1")
        self.assertEqual(self.model._workspaces, ["ws1"])
        self.model.add_workspace("ws1")
        self.assertEqual(self.model._workspaces, ["ws1"])
        self.model.add_workspace("ws2")
        self.assertEqual(self.model._workspaces, ["ws1", "ws2"])
        self.mMtd.__getitem__.return_value = mock.Mock(spec=WorkspaceGroup)
        self.mMtd.__getitem__.return_value.getNames.return_value = ["ws2",
                                                                    "ws3",
                                                                    "ws4"]
        self.model.add_workspace("g1")
        self.assertEqual(self.model._workspaces, ["ws1", "ws2", "ws3", "ws4"])

    def test_delWorkspace(self):
        self.model._workspaces = ["ws1", "ws2", "ws3"]
        self.model._plottedData = [("ws1", 1), ("ws2", 2)]
        self.model._plotMode = self.model.BIN_MODE
        self.model.del_workspace("ws4")
        self.assertEqual(self.model._workspaces, ["ws1", "ws2", "ws3"])
        self.assertEqual(self.model._plottedData, [("ws1", 1), ("ws2", 2)])
        self.assertEqual(self.model._plotMode, self.model.BIN_MODE)
        self.model.del_workspace("ws3")
        self.assertEqual(self.model._workspaces, ["ws1", "ws2"])
        self.assertEqual(self.model._plottedData, [("ws1", 1), ("ws2", 2)])
        self.assertEqual(self.model._plotMode, self.model.BIN_MODE)
        self.model.del_workspace("ws2")
        self.assertEqual(self.model._workspaces, ["ws1"])
        self.assertEqual(self.model._plottedData, [("ws1", 1)])
        self.assertEqual(self.model._plotMode, self.model.BIN_MODE)
        self.model.del_workspace("ws1")
        self.assertEqual(self.model._workspaces, [])
        self.assertEqual(self.model._plottedData, [])
        self.assertIsNone(self.model._plotMode)

    def test_getWorkspaces(self):
        self.model._workspaces = ["ws1", "ws2", "ws3"]
        wsList = self.model.get_workspaces()
        self.assertEqual(wsList, self.model._workspaces)
        # test that getWorkspace is returning a copy
        wsList = wsList[0:2]
        self.assertEqual(wsList, ["ws1", "ws2"])
        self.assertEqual(self.model._workspaces, ["ws1", "ws2", "ws3"])

    def test_setBinMode(self):
        self.assertIsNone(self.model._plotMode)
        self.model.set_bin_mode()
        self.assertEqual(self.model._plotMode, self.model.BIN_MODE)
        self.model._plotMode = self.model.SPECTRUM_MODE
        self.model.set_bin_mode()
        self.assertEqual(self.model._plotMode, self.model.BIN_MODE)

    def test_setSpectrumMode(self):
        self.assertIsNone(self.model._plotMode)
        self.model.set_spectrum_mode()
        self.assertEqual(self.model._plotMode, self.model.SPECTRUM_MODE)
        self.model._plotMode = self.model.BIN_MODE
        self.model.set_spectrum_mode()
        self.assertEqual(self.model._plotMode, self.model.SPECTRUM_MODE)

    def test_isBinMode(self):
        self.assertIsNone(self.model._plotMode)
        self.assertFalse(self.model.is_bin_mode())
        self.model._plotMode = self.model.BIN_MODE
        self.assertTrue(self.model.is_bin_mode())
        self.model._plotMode = self.model.SPECTRUM_MODE
        self.assertFalse(self.model.is_bin_mode())

    def test_isSpectrumMode(self):
        self.assertIsNone(self.model._plotMode)
        self.assertFalse(self.model.is_spectrum_mode())
        self.model._plotMode = self.model.SPECTRUM_MODE
        self.assertTrue(self.model.is_spectrum_mode())
        self.model._plotMode = self.model.BIN_MODE
        self.assertFalse(self.model.is_spectrum_mode())

    def test_addData(self):
        self.assertEqual(self.model._plottedData, [])
        self.model.add_data("ws1", 1)
        self.assertEqual(self.model._plottedData, [("ws1", 1)])
        self.model.add_data("ws1", 2)
        self.assertEqual(self.model._plottedData, [("ws1", 1), ("ws1", 2)])
        self.model.add_data("ws1", 1)
        self.assertEqual(self.model._plottedData, [("ws1", 1), ("ws1", 2)])

    def test_removeData(self):
        self.assertEqual(self.model._plottedData, [])
        self.model.remove_data("ws1", 1)
        self.assertEqual(self.model._plottedData, [])
        self.model._plottedData = [("ws1", 1), ("ws1", 2), ("ws2", 1)]
        self.model._plotMode = self.model.BIN_MODE
        self.model.remove_data("ws1", 1)
        self.assertEqual(self.model._plottedData, [("ws1", 2), ("ws2", 1)])
        self.assertEqual(self.model._plotMode, self.model.BIN_MODE)
        self.model.remove_data("ws2", 1)
        self.assertEqual(self.model._plottedData, [("ws1", 2)])
        self.assertEqual(self.model._plotMode, self.model.BIN_MODE)
        self.model.remove_data("ws1", 2)
        self.assertEqual(self.model._plottedData, [])
        self.assertIsNone(self.model._plotMode)

    def test_getPlottedData(self):
        self.assertEqual(self.model._plottedData, [])
        self.assertEqual(self.model.get_plotted_data(), [])
        self.model._plottedData = [("ws1", 1), ("ws1", 2), ("ws2", 1)]
        data = self.model.get_plotted_data()
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
        self.model.on_workspace_deleted("ws4")
        self.assertEqual(self.model._workspaces, ["ws1", "ws2", "ws3"])
        self.assertEqual(self.model._plottedData,
                         [("ws1", 1), ("ws1", 3), ("ws3", 10)])
        self.assertEqual(self.model._plotMode, self.model.SPECTRUM_MODE)
        self.model.workspaceDeleted.emit.assert_not_called()
        self.model.on_workspace_deleted("ws1")
        self.assertEqual(self.model._workspaces, ["ws2", "ws3"])
        self.assertEqual(self.model._plottedData, [("ws3", 10)])
        self.assertEqual(self.model._plotMode, self.model.SPECTRUM_MODE)
        self.model.workspaceDeleted.emit.assert_called_once_with("ws1")
        self.model.workspaceDeleted.reset_mock()
        self.model.on_workspace_deleted("ws2")
        self.assertEqual(self.model._workspaces, ["ws3"])
        self.assertEqual(self.model._plottedData, [("ws3", 10)])
        self.assertEqual(self.model._plotMode, self.model.SPECTRUM_MODE)
        self.model.workspaceDeleted.emit.assert_called_once_with("ws2")
        self.model.workspaceDeleted.reset_mock()
        self.model.on_workspace_deleted("ws3")
        self.assertEqual(self.model._workspaces, [])
        self.assertEqual(self.model._plottedData, [])
        self.assertIsNone(self.model._plotMode)
        self.model.workspaceDeleted.emit.assert_called_once_with("ws3")
        self.model.workspaceDeleted.reset_mock()

    def test_onWorkspaceRenamed(self):
        self.model._workspaces = ["ws1", "ws2", "ws3"]
        self.model._plottedData = [("ws1", 1), ("ws1", 3), ("ws3", 10)]
        self.model.workspaceRenamed = mock.Mock()
        self.model.on_workspace_renamed("ws4", "ws5")
        self.assertEqual(self.model._workspaces, ["ws1", "ws2", "ws3"])
        self.assertEqual(self.model._plottedData,
                         [("ws1", 1), ("ws1", 3), ("ws3", 10)])
        self.model.workspaceRenamed.emit.assert_not_called()
        self.model.on_workspace_renamed("ws1", "ws5")
        self.assertEqual(self.model._workspaces, ["ws5", "ws2", "ws3"])
        self.assertEqual(self.model._plottedData,
                         [("ws5", 1), ("ws5", 3), ("ws3", 10)])
        self.model.workspaceRenamed.emit.assert_called_once_with("ws1", "ws5")

    def test_onWorkspaceReplaced(self):
        self.model._workspaces = ["ws1", "ws2", "ws3"]
        self.model.workspaceReplaced = mock.Mock()
        self.model.on_workspace_replaced("ws4")
        self.model.workspaceReplaced.emit.assert_not_called()
        self.model.on_workspace_replaced("ws1")
        self.model.workspaceReplaced.emit.assert_called_once_with("ws1")
