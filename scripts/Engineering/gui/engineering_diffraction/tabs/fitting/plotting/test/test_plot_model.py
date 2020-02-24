# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +

import unittest

from mantid.py3compat import mock

from Engineering.gui.engineering_diffraction.tabs.fitting.plotting import plot_model

dir_path = "Engineering.gui.engineering_diffraction.tabs.fitting.plotting"


class FittingPlotModelTest(unittest.TestCase):
    def setUp(self):
        self.model = plot_model.FittingPlotModel()

    def test_adding_workspace_to_plot(self):
        self.assertEqual(set(), self.model.plotted_workspaces)
        ax = mock.MagicMock()

        self.model.add_workspace_to_plot("mocked_ws", ax, {"linestyle": "x"})

        self.assertEqual({"mocked_ws"}, self.model.plotted_workspaces)
        ax.plot.assert_called_once_with("mocked_ws", linestyle="x")

    def test_removing_single_tracked_workspace_from_plot(self):
        self.model.plotted_workspaces.add("mocked_ws")
        ax = mock.MagicMock()

        self.model.remove_workspace_from_plot("mocked_ws", ax)

        self.assertEqual(set(), self.model.plotted_workspaces)
        ax.remove_workspace_artists.assert_called_once_with("mocked_ws")

    def test_removing_not_tracked_workspace_from_plot(self):
        self.model.plotted_workspaces.add("mocked_ws")
        ax = mock.MagicMock()

        self.model.remove_workspace_from_plot("whatever", ax)

        self.assertEqual({"mocked_ws"}, self.model.plotted_workspaces)
        ax.remove_workspace_artists.assert_not_called()

    def test_removing_all_workspaces_from_plot(self):
        self.model.plotted_workspaces.update({"mocked_ws", "mock_ws_2"})
        ax = mock.MagicMock()

        self.model.remove_all_workspaces_from_plot(ax)

        self.assertEqual(set(), self.model.plotted_workspaces)
        self.assertEqual(1, ax.cla.call_count)


if __name__ == '__main__':
    unittest.main()
