# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2023 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +

import unittest
from unittest import mock

from mantidqt.gui_helper import get_qapplication

from mantidqtinterfaces.dns_powder_tof.data_structures.dns_widget import DNSWidget
from mantidqtinterfaces.dns_powder_elastic.plot.elastic_powder_plot_model import DNSElasticPowderPlotModel
from mantidqtinterfaces.dns_powder_elastic.plot.elastic_powder_plot_presenter import DNSElasticPowderPlotPresenter
from mantidqtinterfaces.dns_powder_elastic.plot.elastic_powder_plot_view import DNSElasticPowderPlotView
from mantidqtinterfaces.dns_powder_elastic.plot.elastic_powder_plot_widget import DNSElasticPowderPlotWidget

app, within_mantid = get_qapplication()


class DNSElasticPowderPlotWidgetTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        parent = mock.Mock()
        parent.view = None
        cls.widget = DNSElasticPowderPlotWidget("elastic_powder_plot", parent)

    def test___init__(self):
        self.assertIsInstance(self.widget, DNSElasticPowderPlotWidget)
        self.assertIsInstance(self.widget, DNSWidget)
        self.assertIsInstance(self.widget.view, DNSElasticPowderPlotView)
        self.assertIsInstance(self.widget.model, DNSElasticPowderPlotModel)
        self.assertIsInstance(self.widget.presenter, DNSElasticPowderPlotPresenter)


if __name__ == "__main__":
    unittest.main()
