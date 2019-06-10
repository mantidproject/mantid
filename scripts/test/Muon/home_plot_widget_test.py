# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
import unittest
from PyQt4 import QtGui

from mantid.py3compat import mock

from Muon.GUI.Common.home_plot_widget.home_plot_widget_model import HomePlotWidgetModel
from Muon.GUI.Common.home_plot_widget.home_plot_widget_presenter import HomePlotWidgetPresenter



class HomeTabPlotPresenterTest(unittest.TestCase):
    def setUp(self):
        self.context = mock.MagicMock()
        self.plotting_window_model = mock.MagicMock()
        self.view = mock.MagicMock()
        self.model = mock.MagicMock()
        self.presenter = HomePlotWidgetPresenter(self.view, self.model)

    def test_use_rebin_changed_resets_use_raw_to_true_if_no_rebin_specified(self):
        self.view.if_raw.return_value = False
        self.model.context._do_rebin.return_value = False

        self.presenter.handle_use_raw_workspaces_changed()

        self.model.plot.assert_not_called()
        self.view.warning_popup.assert_called_once_with('No rebin options specified')

    def test_use_rebin_changed_replots_figure_with_appropriate_data(self):
        self.view.if_raw.return_value = False
        self.view.get_selected.return_value = 'Asymmetry'
        self.model.context._do_rebin.return_value = False

        self.presenter.handle_use_raw_workspaces_changed()

        self.model.plot.assert_called_once_with('Asymmetry', False)

if __name__ == '__main__':
    unittest.main(buffer=False, verbosity=2)
