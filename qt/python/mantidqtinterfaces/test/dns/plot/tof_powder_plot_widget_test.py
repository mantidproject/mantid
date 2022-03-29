# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
import unittest
from unittest import mock

from mantidqt.gui_helper import get_qapplication
from mantidqtinterfaces.dns.data_structures.dns_widget import \
    DNSWidget
from mantidqtinterfaces.dns.plot.tof_powder_plot_model import \
    DNSTofPowderPlotModel
from mantidqtinterfaces.dns.plot.tof_powder_plot_presenter import \
    DNSTofPowderPlotPresenter
from mantidqtinterfaces.dns.plot.tof_powder_plot_view import \
    DNSTofPowderPlotView
from mantidqtinterfaces.dns.plot.tof_powder_plot_widget import \
    DNSTofPowderPlotWidget

app, within_mantid = get_qapplication()


class DNSTofPowderPlotWidgetTest(unittest.TestCase):

    @classmethod
    def setUpClass(cls):
        parent = mock.Mock()
        parent.view = None
        cls.widget = DNSTofPowderPlotWidget('tof_powder_plot', parent)

    def test___init__(self):
        self.assertIsInstance(self.widget, DNSTofPowderPlotWidget)
        self.assertIsInstance(self.widget, DNSWidget)
        self.assertIsInstance(self.widget.view, DNSTofPowderPlotView)
        self.assertIsInstance(self.widget.model, DNSTofPowderPlotModel)
        self.assertIsInstance(self.widget.presenter, DNSTofPowderPlotPresenter)


if __name__ == '__main__':
    unittest.main()
