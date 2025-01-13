# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +

import unittest
from unittest import mock
from unittest.mock import patch

from mantidqtinterfaces.dns_powder_tof.data_structures.dns_observer import DNSObserver
from mantidqtinterfaces.dns_powder_tof.plot.tof_powder_plot_model import DNSTofPowderPlotModel
from mantidqtinterfaces.dns_powder_tof.plot.tof_powder_plot_presenter import DNSTofPowderPlotPresenter
from mantidqtinterfaces.dns_powder_tof.plot.tof_powder_plot_view import DNSTofPowderPlotView


class DNSTofPowderPlotPresenterTest(unittest.TestCase):
    # pylint: disable=protected-access
    view = None
    model = None
    parent = None
    presenter = None

    @classmethod
    def setUpClass(cls):
        cls.parent = mock.Mock()
        cls.view = mock.create_autospec(DNSTofPowderPlotView, instance=True)
        cls.model = mock.create_autospec(DNSTofPowderPlotModel, instance=True)
        cls.presenter = DNSTofPowderPlotPresenter(view=cls.view, model=cls.model, parent=cls.parent)
        cls.presenter.param_dict = {"tof_powder_script_generator": {"script_number": 3}}

    def setUp(self):
        self.model.reset_mock()
        self.view.reset_mock()
        self.presenter._plotted_script_number = 0

    def test___init__(self):
        self.assertIsInstance(self.presenter, DNSTofPowderPlotPresenter)
        self.assertIsInstance(self.presenter, DNSObserver)
        self.assertEqual(self.presenter._plotted_script_number, 0)

    def test_plot(self):
        self.model.get_plot_workspace.return_value = "abcd"
        self.presenter._plot()
        self.model.get_plot_workspace.assert_called_once()
        self.view.set_plot.assert_called_once_with("abcd")
        self.assertEqual(self.presenter._plotted_script_number, 3)
        self.model.get_plot_workspace()
        self.model.reset_mock()
        self.view.reset_mock()
        self.model.get_plot_workspace.return_value = ""
        self.presenter._plot()
        self.model.get_plot_workspace.assert_called_once()
        self.view.set_plot.assert_not_called()
        self.view.raise_error.assert_called_once()

    @patch("mantidqtinterfaces.dns_powder_tof.plot.tof_powder_plot_presenter.DNSTofPowderPlotPresenter._plot")
    def test_tab_got_focus(self, mock_plot):
        self.presenter.tab_got_focus()
        mock_plot.assert_called_once()


if __name__ == "__main__":
    unittest.main()
