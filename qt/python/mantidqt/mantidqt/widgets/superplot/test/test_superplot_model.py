# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +

import unittest
from unittest import mock

from mantid.api import WorkspaceGroup

from mantidqt.widgets.superplot.model import SuperplotModel


class SuperplotModelTest(unittest.TestCase):

    def setUp(self):
        py_module = "mantidqt.widgets.superplot.model"

        patch = mock.patch(py_module + ".SuperplotAdsObserver")
        self.m_obs = patch.start()
        self.m_obs = self.m_obs.return_value
        self.addCleanup(patch.stop)

        patch = mock.patch(py_module + ".mtd")
        self.m_mtd = patch.start()
        self.addCleanup(patch.stop)

        self.model = SuperplotModel()

    def test_init(self):
        self.assertEqual(self.model._workspaces, [])
        self.assertEqual(self.model._plotted_data, [])
        self.assertEqual(self.model._ws_colors, {})
        self.assertIsNone(self.model._plot_mode)
        self.m_obs.signals.sig_ws_deleted.connect.assert_called_once()
        self.m_obs.signals.sig_ws_renamed.connect.assert_called_once()
        self.m_obs.signals.sig_ws_replaced.connect.assert_called_once()

    def test_add_workspace(self):
        self.m_mtd.__contains__.return_value = False
        self.model.add_workspace("ws1")
        self.assertEqual(self.model._workspaces, [])
        self.m_mtd.__contains__.return_value = True
        self.model.add_workspace("ws1")
        self.assertEqual(self.model._workspaces, ["ws1"])
        self.model.add_workspace("ws1")
        self.assertEqual(self.model._workspaces, ["ws1"])
        self.model.add_workspace("ws2")
        self.assertEqual(self.model._workspaces, ["ws1", "ws2"])
        self.m_mtd.__getitem__.return_value = mock.Mock(spec=WorkspaceGroup)
        self.m_mtd.__getitem__.return_value.getNames.return_value = ["ws2",
                                                                     "ws3",
                                                                     "ws4"]
        self.model.add_workspace("g1")
        self.assertEqual(self.model._workspaces, ["ws1", "ws2", "ws3", "ws4"])

    def test_del_workspace(self):
        self.model._workspaces = ["ws1", "ws2", "ws3"]
        self.model._plotted_data = [("ws1", 1), ("ws2", 2)]
        self.model._plot_mode = self.model.BIN_MODE
        self.model.del_workspace("ws4")
        self.assertEqual(self.model._workspaces, ["ws1", "ws2", "ws3"])
        self.assertEqual(self.model._plotted_data, [("ws1", 1), ("ws2", 2)])
        self.assertEqual(self.model._plot_mode, self.model.BIN_MODE)
        self.model.del_workspace("ws3")
        self.assertEqual(self.model._workspaces, ["ws1", "ws2"])
        self.assertEqual(self.model._plotted_data, [("ws1", 1), ("ws2", 2)])
        self.assertEqual(self.model._plot_mode, self.model.BIN_MODE)
        self.model.del_workspace("ws2")
        self.assertEqual(self.model._workspaces, ["ws1"])
        self.assertEqual(self.model._plotted_data, [("ws1", 1)])
        self.assertEqual(self.model._plot_mode, self.model.BIN_MODE)
        self.model.del_workspace("ws1")
        self.assertEqual(self.model._workspaces, [])
        self.assertEqual(self.model._plotted_data, [])
        self.assertIsNone(self.model._plot_mode)

    def test_get_workspaces(self):
        self.model._workspaces = ["ws1", "ws2", "ws3"]
        wsList = self.model.get_workspaces()
        self.assertEqual(wsList, self.model._workspaces)
        # test that getWorkspace is returning a copy
        wsList = wsList[0:2]
        self.assertEqual(wsList, ["ws1", "ws2"])
        self.assertEqual(self.model._workspaces, ["ws1", "ws2", "ws3"])

    def test_set_workspace_color(self):
        self.assertEqual(self.model._ws_colors, {})
        self.model.set_workspace_color("ws1", "color1")
        self.assertDictEqual(self.model._ws_colors, {"ws1": "color1"})
        self.model.set_workspace_color("ws2", "color2")
        self.assertDictEqual(self.model._ws_colors, {"ws1": "color1",
                                                     "ws2": "color2"})
        self.model.set_workspace_color("ws1", "color3")
        self.assertDictEqual(self.model._ws_colors, {"ws1": "color3",
                                                     "ws2": "color2"})

    def test_get_workspace_color(self):
        self.model._ws_colors = {"ws1": "color1", "ws2": "color2"}
        color = self.model.get_workspace_color("ws1")
        self.assertEqual(color, "color1")
        color = self.model.get_workspace_color("ws3")
        self.assertIsNone(color)

    def test_set_bin_mode(self):
        self.assertIsNone(self.model._plot_mode)
        self.model.set_bin_mode()
        self.assertEqual(self.model._plot_mode, self.model.BIN_MODE)
        self.model._plot_mode = self.model.SPECTRUM_MODE
        self.model.set_bin_mode()
        self.assertEqual(self.model._plot_mode, self.model.BIN_MODE)

    def test_set_spectrum_mode(self):
        self.assertIsNone(self.model._plot_mode)
        self.model.set_spectrum_mode()
        self.assertEqual(self.model._plot_mode, self.model.SPECTRUM_MODE)
        self.model._plot_mode = self.model.BIN_MODE
        self.model.set_spectrum_mode()
        self.assertEqual(self.model._plot_mode, self.model.SPECTRUM_MODE)

    def test_is_bin_mode(self):
        self.assertIsNone(self.model._plot_mode)
        self.assertFalse(self.model.is_bin_mode())
        self.model._plot_mode = self.model.BIN_MODE
        self.assertTrue(self.model.is_bin_mode())
        self.model._plot_mode = self.model.SPECTRUM_MODE
        self.assertFalse(self.model.is_bin_mode())

    def test_is_spectrum_mode(self):
        self.assertIsNone(self.model._plot_mode)
        self.assertFalse(self.model.is_spectrum_mode())
        self.model._plot_mode = self.model.SPECTRUM_MODE
        self.assertTrue(self.model.is_spectrum_mode())
        self.model._plot_mode = self.model.BIN_MODE
        self.assertFalse(self.model.is_spectrum_mode())

    def test_add_data(self):
        self.assertEqual(self.model._plotted_data, [])
        self.model.add_data("ws1", 1)
        self.assertEqual(self.model._plotted_data, [("ws1", 1)])
        self.model.add_data("ws1", 2)
        self.assertEqual(self.model._plotted_data, [("ws1", 1), ("ws1", 2)])
        self.model.add_data("ws1", 1)
        self.assertEqual(self.model._plotted_data, [("ws1", 1), ("ws1", 2)])

    def test_remove_data(self):
        self.assertEqual(self.model._plotted_data, [])
        self.model.remove_data("ws1", 1)
        self.assertEqual(self.model._plotted_data, [])
        self.model._plotted_data = [("ws1", 1), ("ws1", 2), ("ws2", 1)]
        self.model._plot_mode = self.model.BIN_MODE
        self.model.remove_data("ws1", 1)
        self.assertEqual(self.model._plotted_data, [("ws1", 2), ("ws2", 1)])
        self.assertEqual(self.model._plot_mode, self.model.BIN_MODE)
        self.model.remove_data("ws2", 1)
        self.assertEqual(self.model._plotted_data, [("ws1", 2)])
        self.assertEqual(self.model._plot_mode, self.model.BIN_MODE)
        self.model.remove_data("ws1", 2)
        self.assertEqual(self.model._plotted_data, [])
        self.assertIsNone(self.model._plot_mode)

    def test_get_plotted_data(self):
        self.assertEqual(self.model._plotted_data, [])
        self.assertEqual(self.model.get_plotted_data(), [])
        self.model._plotted_data = [("ws1", 1), ("ws1", 2), ("ws2", 1)]
        data = self.model.get_plotted_data()
        self.assertEqual(data, [("ws1", 1), ("ws1", 2), ("ws2", 1)])
        data = data[0:2]
        self.assertEqual(data, [("ws1", 1), ("ws1", 2)])
        self.assertEqual(self.model._plotted_data,
                         [("ws1", 1), ("ws1", 2), ("ws2", 1)])

    def test_on_workspace_deleted(self):
        self.model._workspaces = ["ws1", "ws2", "ws3"]
        self.model._plotted_data = [("ws1", 1), ("ws1", 3), ("ws3", 10)]
        self.model._plot_mode = self.model.SPECTRUM_MODE
        self.model.sig_workspace_deleted = mock.Mock()
        self.model.on_workspace_deleted("ws4")
        self.assertEqual(self.model._workspaces, ["ws1", "ws2", "ws3"])
        self.assertEqual(self.model._plotted_data,
                         [("ws1", 1), ("ws1", 3), ("ws3", 10)])
        self.assertEqual(self.model._plot_mode, self.model.SPECTRUM_MODE)
        self.model.sig_workspace_deleted.emit.assert_not_called()
        self.model.on_workspace_deleted("ws1")
        self.assertEqual(self.model._workspaces, ["ws2", "ws3"])
        self.assertEqual(self.model._plotted_data, [("ws3", 10)])
        self.assertEqual(self.model._plot_mode, self.model.SPECTRUM_MODE)
        self.model.sig_workspace_deleted.emit.assert_called_once_with("ws1")
        self.model.sig_workspace_deleted.reset_mock()
        self.model.on_workspace_deleted("ws2")
        self.assertEqual(self.model._workspaces, ["ws3"])
        self.assertEqual(self.model._plotted_data, [("ws3", 10)])
        self.assertEqual(self.model._plot_mode, self.model.SPECTRUM_MODE)
        self.model.sig_workspace_deleted.emit.assert_called_once_with("ws2")
        self.model.sig_workspace_deleted.reset_mock()
        self.model.on_workspace_deleted("ws3")
        self.assertEqual(self.model._workspaces, [])
        self.assertEqual(self.model._plotted_data, [])
        self.assertIsNone(self.model._plot_mode)
        self.model.sig_workspace_deleted.emit.assert_called_once_with("ws3")
        self.model.sig_workspace_deleted.reset_mock()

    def test_on_workspace_renamed(self):
        self.model._workspaces = ["ws1", "ws2", "ws3"]
        self.model._plotted_data = [("ws1", 1), ("ws1", 3), ("ws3", 10)]
        self.model.sig_workspace_renamed = mock.Mock()
        self.model.on_workspace_renamed("ws4", "ws5")
        self.assertEqual(self.model._workspaces, ["ws1", "ws2", "ws3"])
        self.assertEqual(self.model._plotted_data,
                         [("ws1", 1), ("ws1", 3), ("ws3", 10)])
        self.model.sig_workspace_renamed.emit.assert_not_called()
        self.model.on_workspace_renamed("ws1", "ws5")
        self.assertEqual(self.model._workspaces, ["ws5", "ws2", "ws3"])
        self.assertEqual(self.model._plotted_data,
                         [("ws5", 1), ("ws5", 3), ("ws3", 10)])
        self.model.sig_workspace_renamed.emit.assert_called_once_with("ws1", "ws5")

    def test_on_workspace_replaced(self):
        self.model._workspaces = ["ws1", "ws2", "ws3"]
        self.model.sig_workspace_replaced = mock.Mock()
        self.model.on_workspace_replaced("ws4")
        self.model.sig_workspace_replaced.emit.assert_not_called()
        self.model.on_workspace_replaced("ws1")
        self.model.sig_workspace_replaced.emit.assert_called_once_with("ws1")
