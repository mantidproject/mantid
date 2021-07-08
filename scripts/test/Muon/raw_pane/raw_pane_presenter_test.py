# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import unittest
from unittest import mock

from Muon.GUI.Common.plot_widget.external_plotting.external_plotting_model import ExternalPlottingModel
from Muon.GUI.Common.plot_widget.external_plotting.external_plotting_view import ExternalPlottingView
from Muon.GUI.Common.plot_widget.base_pane.base_pane_model import RawPaneModel
from Muon.GUI.Common.plot_widget.base_pane.base_pane_presenter import RawPanePresenter
from Muon.GUI.Common.plot_widget.base_pane.base_pane_view import RawPaneView
from Muon.GUI.Common.plot_widget.plotting_canvas.plotting_canvas_presenter_interface import \
    PlottingCanvasPresenterInterface
from mantid import AnalysisDataService
from mantid.simpleapi import CreateWorkspace
from mantidqt.utils.qt.testing import start_qapplication


@start_qapplication
class RawPanePresenterTest(unittest.TestCase):

    def setUp(self):
        self.context = mock.MagicMock()
        self.model = mock.Mock(spec=RawPaneModel)
        self.model.name = "test"
        self.view = mock.Mock(spec=RawPaneView)
        self.view.warning_popup = mock.MagicMock()
        self.view.setEnabled = mock.MagicMock()
        self.external_plotting_model = mock.Mock(spec=ExternalPlottingModel)
        self.external_plotting_view = mock.Mock(spec=ExternalPlottingView)
        self.figure_presenter = mock.Mock(spec=PlottingCanvasPresenterInterface)
        self.figure_presenter.force_autoscale = mock.Mock()

        self.context.group_pair_context.selected_groups = ['bottom']
        self.context.group_pair_context.selected_pairs = []

        self.presenter = RawPanePresenter(view=self.view, model=self.model, context=self.context,
                                                   figure_presenter=self.figure_presenter,
                                                   external_plotting_view=self.external_plotting_view,
                                                   external_plotting_model=self.external_plotting_model)

    def create_workspace(self, name):
        x_range = range(1, 100)
        y_range = [x * x for x in x_range]
        return CreateWorkspace(DataX=x_range, DataY=y_range, OutputWorkspace=name)

    def tearDown(self):
        AnalysisDataService.Instance().clear()


if __name__ == '__main__':
    unittest.main(buffer=False, verbosity=2)
