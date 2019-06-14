# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
import unittest

from mantid.py3compat import mock
from mantidqt.utils.qt.testing import GuiTest
from qtpy.QtWidgets import QWidget

from Muon.GUI.Common.home_plot_widget.home_plot_widget_model import HomePlotWidgetModel
from Muon.GUI.Common.home_plot_widget.home_plot_widget_presenter import HomePlotWidgetPresenter
from Muon.GUI.Common.home_plot_widget.home_plot_widget_view import HomePlotWidgetView


class HomeTabPlotPresenterTest(GuiTest):
    def setUp(self):
        self.obj = QWidget()
        self.view = HomePlotWidgetView(self.obj)
        self.model = HomePlotWidgetModel()
        self.presenter = HomePlotWidgetPresenter(self.view, self.model)

        self.view.warning_popup = mock.MagicMock()

    def tearDown(self):
        self.obj = None

    def test_plot_button_produces_not_implemented_method(self):
        self.view.plot_button.clicked.emit(True)

        self.view.warning_popup.assert_called_once_with('Plotting not currently implemented!')


if __name__ == '__main__':
    unittest.main(buffer=False, verbosity=2)
