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
from mantidqtinterfaces.dns.dns_modus import DNSModus
from mantidqtinterfaces.dns.main_presenter import \
    DNSReductionGUIPresenter
from mantidqtinterfaces.dns.main_view import DNSReductionGUIView
from mantidqtinterfaces.dns.main_widget import DNSReductionGuiWidget
from mantidqtinterfaces.dns.parameter_abo import ParameterAbo

app, within_mantid = get_qapplication()


class DNSReductionGUIWidgetTest(unittest.TestCase):

    @classmethod
    def setUpClass(cls):
        parent = mock.Mock()
        parent.view = None
        cls.widget = DNSReductionGuiWidget('DNS_reduction_gui', parent)

    def test___init__(self):
        self.assertIsInstance(self.widget, DNSReductionGuiWidget)
        self.assertIsInstance(self.widget, DNSWidget)
        self.assertIsInstance(self.widget.view, DNSReductionGUIView)
        self.assertIsInstance(self.widget.model, ParameterAbo)
        self.assertIsInstance(self.widget.modus, DNSModus)
        self.assertIsInstance(self.widget.presenter, DNSReductionGUIPresenter)
        self.assertTrue(hasattr(self.widget, 'name'))


if __name__ == '__main__':
    unittest.main()
