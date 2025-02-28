# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import unittest

from unittest import mock
from mantidqtinterfaces.MultiPlotting.QuickEdit.quickEdit_presenter import QuickEditPresenter
from mantidqtinterfaces.MultiPlotting.QuickEdit.quickEdit_view import QuickEditView


class QuickEditPresenterTest(unittest.TestCase):
    def setUp(self):
        self.view = mock.create_autospec(QuickEditView, instance=True)
        self.pres = QuickEditPresenter(self.view)
        self.slot = mock.Mock()

    def test_add_subplot(self):
        name = "new plot"
        self.pres.add_subplot(name)
        self.assertEqual(self.view.add_subplot.call_count, 1)
        self.assertEqual(self.view.find_index.call_count, 1)
        self.assertEqual(self.view.set_index.call_count, 1)

    def test_connect_autoscale(self):
        self.pres.connect_autoscale_changed(self.slot)
        self.view.connect_autoscale_changed.assert_called_with(self.slot)

    def test_connect_errors(self):
        self.pres.connect_errors_changed(self.slot)
        self.view.connect_errors_changed.assert_called_with(self.slot)

    def test_connect_x_range(self):
        self.pres.connect_x_range_changed(self.slot)
        self.view.connect_x_range_changed.assert_called_with(self.slot)

    def test_connect_y_range(self):
        self.pres.connect_y_range_changed(self.slot)
        self.view.connect_y_range_changed.assert_called_with(self.slot)

    def test_connect_plot_selection(self):
        self.pres.connect_plot_selection(self.slot)
        self.view.connect_plot_selection.assert_called_with(self.slot)

    def test_all(self):
        names = ["all", "one", "two"]

        def plots(index):
            return names[index]

        self.view.number_of_plots = mock.MagicMock(return_value=len(names))
        self.view.plot_at_index = mock.MagicMock(side_effect=plots)
        output = self.pres.all()
        # should miss out all
        for k in range(len(output)):
            self.assertEqual(names[k + 1], output[k])

    def test_set_plot_x_range(self):
        self.pres.set_plot_x_range([0, 1])
        self.view.set_plot_x_range.assert_called_with([0, 1])

    def test_set_plot_y_range(self):
        self.pres.set_plot_y_range([0, 1])
        self.view.set_plot_y_range.assert_called_with([0, 1])

    def test_set_errors_TT(self):
        self.view.get_errors = mock.MagicMock(return_value=True)
        self.pres.set_errors(True)
        self.assertEqual(self.view.set_errors.call_count, 0)

    def test_set_errors_TF(self):
        self.view.get_errors = mock.MagicMock(return_value=True)
        self.pres.set_errors(False)
        self.assertEqual(self.view.set_errors.call_count, 1)
        self.view.set_errors.assert_called_with(False)

    def test_set_errors_FT(self):
        self.view.get_errors = mock.MagicMock(return_value=False)
        self.pres.set_errors(True)
        self.assertEqual(self.view.set_errors.call_count, 1)
        self.view.set_errors.assert_called_with(True)

    def test_set_errors_FF(self):
        self.view.get_errors = mock.MagicMock(return_value=False)
        self.pres.set_errors(False)
        self.assertEqual(self.view.set_errors.call_count, 0)

    def test_rmSubplot(self):
        self.pres.rm_subplot("test")

        self.assertEqual(self.view.rm_subplot.call_count, 1)
        self.assertEqual(self.view.find_index.call_count, 2)
        self.assertEqual(self.view.set_index.call_count, 1)


if __name__ == "__main__":
    unittest.main()
