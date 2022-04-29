# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
import unittest
from unittest import mock

from mantidqt.gui_helper import get_qapplication
from mantidqtinterfaces.dns_powder_tof.data_structures.dns_widget import \
    DNSWidget
from mantidqtinterfaces.dns_powder_tof.xml_dump.xml_dump_model import \
    DNSXMLDumpModel
from mantidqtinterfaces.dns_powder_tof.xml_dump.xml_dump_presenter import \
    DNSXMLDumpPresenter
from mantidqtinterfaces.dns_powder_tof.xml_dump.xml_dump_view import \
    DNSXMLDumpView
from mantidqtinterfaces.dns_powder_tof.xml_dump.xml_dump_widget import \
    DNSXMLDumpWidget

app, within_mantid = get_qapplication()


class DNSXMLDump_widgetTest(unittest.TestCase):

    @classmethod
    def setUpClass(cls):
        parent = mock.Mock()
        parent.view = None
        cls.widget = DNSXMLDumpWidget('elastic_powder_options', parent)

    def test___init__(self):
        self.assertIsInstance(self.widget, DNSXMLDumpWidget)
        self.assertIsInstance(self.widget, DNSWidget)
        self.assertIsInstance(self.widget.view, DNSXMLDumpView)
        self.assertIsInstance(self.widget.model, DNSXMLDumpModel)
        self.assertIsInstance(self.widget.presenter, DNSXMLDumpPresenter)


if __name__ == '__main__':
    unittest.main()
