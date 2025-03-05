# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import unittest

from unittest import mock
from mantidqt.utils.qt.testing import start_qapplication

from mantidqtinterfaces.MultiPlotting.QuickEdit.quickEdit_presenter import QuickEditPresenter
from mantidqtinterfaces.MultiPlotting.QuickEdit.quickEdit_widget import QuickEditWidget


@start_qapplication
class QuickEditWidgetTest(unittest.TestCase):
    def setUp(self):
        self.pres = mock.create_autospec(QuickEditPresenter, instance=True)
        self.widget = QuickEditWidget()
        self.slot = mock.Mock()
        self.widget.set_mock(self.pres)

    def test_connect_autoscale(self):
        self.widget.connect_autoscale_changed(self.slot)
        self.pres.connect_autoscale_changed.assert_called_with(self.slot)

    def test_connect_errors(self):
        self.widget.connect_errors_changed(self.slot)
        self.pres.connect_errors_changed.assert_called_with(self.slot)

    def test_connect_x_range(self):
        self.widget.connect_x_range_changed(self.slot)
        self.pres.connect_x_range_changed.assert_called_with(self.slot)

    def test_connect_y_range(self):
        self.widget.connect_y_range_changed(self.slot)
        self.pres.connect_y_range_changed.assert_called_with(self.slot)

    def test_connect_plot_selection(self):
        self.widget.connect_plot_selection(self.slot)
        self.pres.connect_plot_selection.assert_called_with(self.slot)

    def test_add_subplot(self):
        name = "new plot"
        self.widget.add_subplot(name)
        self.assertEqual(self.pres.add_subplot.call_count, 1)
        self.pres.add_subplot.assert_called_with(name)

    def test_get_selection_one(self):
        name = "one plot"
        self.pres.widget.current_selection = mock.MagicMock(return_value=name)
        output = self.widget.get_selection()
        self.assertEqual([name], output)

    def test_get_selection_all(self):
        name = "All"
        self.pres.widget.current_selection = mock.MagicMock(return_value=name)
        self.widget.get_selection()
        self.assertEqual(self.pres.all.call_count, 1)

    def test_set_plot_x_range(self):
        self.widget.set_plot_x_range([0, 1])
        self.pres.set_plot_x_range.assert_called_with([0, 1])

    def test_set_plot_y_range(self):
        self.widget.set_plot_y_range([0, 1])
        self.pres.set_plot_y_range.assert_called_with([0, 1])

    def test_set_errors_TT(self):
        self.widget.set_errors(True)
        self.pres.set_errors.assert_called_with(True)


if __name__ == "__main__":
    unittest.main()
