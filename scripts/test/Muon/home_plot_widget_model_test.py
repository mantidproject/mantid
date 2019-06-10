# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
import unittest

from mantid.py3compat import mock

from Muon.GUI.Common.home_plot_widget.home_plot_widget_model import HomePlotWidgetModel
from MultiPlotting.multi_plotting_widget import MultiPlotWindow


class HomeTabPlotPresenterTest(unittest.TestCase):
    def setUp(self):
        self.plotting_manager = MultiPlotWindow
        self.context = mock.MagicMock()
        self.model = HomePlotWidgetModel(self.plotting_manager, self.context)

    def test_plot_creates_new_plot_window_and_plots_workspace_list(self):
        self.model.get_workspaces_to_plot.return_value = ['MUSR62260; Group; bottom; Asymmetry; MA',
                                                          'MUSR62260; Group; fwd; Asymmetry; MA']






if __name__ == '__main__':
    unittest.main(buffer=False, verbosity=2)