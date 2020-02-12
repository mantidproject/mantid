# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +

import unittest

from mantid.py3compat import mock

from Engineering.gui.engineering_diffraction.tabs.fitting.plotting import plot_model, plot_view, plot_presenter

dir_path = "Engineering.gui.engineering_diffraction.tabs.fitting.plotting"


class FittingPlotPresenterTest(unittest.TestCase):
    def setUp(self):
        self.model = mock.create_autospec(plot_model.FittingPlotModel)
        self.view = mock.create_autospec(plot_view.FittingPlotView)
        self.presenter = plot_presenter.FittingPlotPresenter(self.model, self.view)

    def test_add_workspace_to_plot(self):
        self.view.get_axes.return_value = ["axis1", "axis2"]

        self.presenter.add_workspace_to_plot("workspace")

        self.assertEqual(1, self.view.update_figure.call_count)
        self.assertEqual(2, self.model.add_workspace_to_plot.call_count)
        self.model.add_workspace_to_plot.assert_any_call("workspace", "axis1", plot_presenter.PLOT_KWARGS)
        self.model.add_workspace_to_plot.assert_any_call("workspace", "axis2", plot_presenter.PLOT_KWARGS)

    def test_remove_workspace_from_plot(self):
        self.view.get_axes.return_value = ["axis1", "axis2"]

        self.presenter.remove_workspace_from_plot("workspace")

        self.assertEqual(1, self.view.update_figure.call_count)
        self.assertEqual(2, self.model.remove_workspace_from_plot.call_count)
        self.model.remove_workspace_from_plot.assert_any_call("workspace", "axis1")
        self.model.remove_workspace_from_plot.assert_any_call("workspace", "axis2")

    def test_clear_plot(self):
        self.view.get_axes.return_value = ["axis1", "axis2"]

        self.presenter.clear_plot()

        self.assertEqual(1, self.view.clear_figure.call_count)
        self.assertEqual(2, self.model.remove_all_workspaces_from_plot.call_count)
        self.model.remove_all_workspaces_from_plot.assert_any_call("axis1")
        self.model.remove_all_workspaces_from_plot.assert_any_call("axis2")


if __name__ == '__main__':
    unittest.main()
