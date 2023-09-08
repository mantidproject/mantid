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
from mantidqtinterfaces.dns_powder_tof.options.common_options_model import DNSCommonOptionsModel
from mantidqtinterfaces.dns_powder_elastic.options.elastic_powder_options_presenter import DNSElasticPowderOptionsPresenter
from mantidqtinterfaces.dns_powder_elastic.options.elastic_powder_options_view import DNSElasticPowderOptionsView
from mantidqtinterfaces.dns_powder_elastic.options.elastic_powder_options_widget import DNSElasticPowderOptionsWidget

app, within_mantid = get_qapplication()


class DNSElasticPowderOptionsWidgetTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        parent = mock.Mock()
        parent.view = None
        cls.widget = DNSElasticPowderOptionsWidget("elastic_powder_options", parent)

    def test___init__(self):
        self.assertIsInstance(self.widget, DNSElasticPowderOptionsWidget)
        self.assertIsInstance(self.widget, DNSWidget)
        self.assertIsInstance(self.widget.view, DNSElasticPowderOptionsView)
        self.assertIsInstance(self.widget.model, DNSCommonOptionsModel)
        self.assertIsInstance(self.widget.presenter, DNSElasticPowderOptionsPresenter)


if __name__ == "__main__":
    unittest.main()
