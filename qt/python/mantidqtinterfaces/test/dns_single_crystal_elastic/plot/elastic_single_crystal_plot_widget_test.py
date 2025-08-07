# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2024 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +

import unittest
from unittest import mock

from mantidqt.gui_helper import get_qapplication
from mantidqtinterfaces.dns_powder_tof.data_structures.dns_widget import DNSWidget
from mantidqtinterfaces.dns_single_crystal_elastic.plot.elastic_single_crystal_plot_model import DNSElasticSCPlotModel
from mantidqtinterfaces.dns_single_crystal_elastic.plot.elastic_single_crystal_plot_presenter import DNSElasticSCPlotPresenter
from mantidqtinterfaces.dns_single_crystal_elastic.plot.elastic_single_crystal_plot_view import DNSElasticSCPlotView
from mantidqtinterfaces.dns_single_crystal_elastic.plot.elastic_single_crystal_plot_widget import DNSElasticSCPlotWidget

app, within_mantid = get_qapplication()


class DNSElasticSCPlotWidgetTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        parent = mock.Mock()
        parent.view = None
        cls.widget = DNSElasticSCPlotWidget("elastic_single_crystal_plot", parent)

    def test___init__(self):
        self.assertIsInstance(self.widget, DNSElasticSCPlotWidget)
        self.assertIsInstance(self.widget, DNSWidget)
        self.assertIsInstance(self.widget.view, DNSElasticSCPlotView)
        self.assertIsInstance(self.widget.model, DNSElasticSCPlotModel)
        self.assertIsInstance(self.widget.presenter, DNSElasticSCPlotPresenter)


if __name__ == "__main__":
    unittest.main()
