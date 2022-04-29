# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
import unittest
from collections import OrderedDict

from mantidqt.gui_helper import get_qapplication
from mantidqtinterfaces.dns_powder_tof.dns_modus import DNSModus
from mantidqtinterfaces.dns_powder_tof.main_widget import DNSReductionGuiWidget

app, within_mantid = get_qapplication()


class DNSModusTest(unittest.TestCase):

    @classmethod
    def setUpClass(cls):
        cls.parent = DNSReductionGuiWidget()
        cls.modus = DNSModus('powder_tof', cls.parent)

    def test___init__(self):
        self.assertIsInstance(self.modus, DNSModus)
        self.assertIsInstance(self.modus, object)
        self.assertEqual(self.modus.name, 'powder_tof')
        self.assertIsInstance(self.modus.widgets, OrderedDict)

    def test_change(self):
        self.modus.change('powder_tof')
        self.assertEqual(self.modus.name, 'powder_tof')
        self.assertEqual(len(self.modus.widgets), 6)


if __name__ == '__main__':
    unittest.main()
