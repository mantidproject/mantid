# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +

import unittest
from collections import OrderedDict
from unittest import mock
from unittest.mock import patch

from mantidqt.gui_helper import get_qapplication
from mantidqtinterfaces.dns_powder_tof.data_structures.dns_view import DNSView
from qtpy.QtWidgets import QCheckBox, QComboBox, QDoubleSpinBox, QGroupBox, QLineEdit, QRadioButton, QSlider, QSpinBox, QToolButton, QWidget

app, within_mantid = get_qapplication()


# needs testing because there is a lot of logic which cannot be
# shifted because of QT types
class DNSViewTest(unittest.TestCase):
    # pylint: disable=protected-access

    @classmethod
    def setUpClass(cls):
        cls.parent = None
        cls.view = DNSView(parent=cls.parent)
        cls.view.app = mock.Mock()
        cls.view.app.processEvents = mock.Mock()
        cls.mock_sb = QSpinBox()
        cls.mock_dsb = QDoubleSpinBox()
        cls.mock_le = QLineEdit()
        cls.mock_cb = QCheckBox()
        cls.mock_rb = QRadioButton()
        cls.mock_combb = QComboBox()
        cls.mock_combb.addItem("A")
        cls.mock_combb.addItem("B")
        cls.mock_sl = QSlider()
        cls.mock_tlb = QToolButton()
        cls.mock_tlb.setCheckable(1)
        cls.mock_gb = QGroupBox()
        cls.mock_gb.setCheckable(1)
        cls.view._map = {"myname": cls.mock_sb}

    def setUp(self):
        self.view.app.reset_mock()

    def test___init__(self):
        self.assertIsInstance(self.view, DNSView)
        self.assertIsInstance(self.view, QWidget)
        self.assertTrue(hasattr(self.view, "HAS_TAB"))
        self.assertTrue(hasattr(self.view, "menus"))
        self.assertTrue(hasattr(self.view, "_map"))

    def test_process_events(self):
        self.view.process_events()
        self.view.app.processEvents.assert_called_once()

    def test_set_single_state_by_name(self):
        self.mock_sb.setValue(0)
        self.view.set_single_state_by_name("myname", 1)
        self.assertEqual(self.mock_sb.value(), 1)

    def test_set_single_state(self):
        self.mock_sb.setValue(0)
        self.mock_dsb.setValue(0)
        self.mock_le.setText("0")
        self.mock_cb.setChecked(0)
        self.mock_rb.setChecked(0)
        self.mock_combb.setCurrentIndex(0)
        self.mock_sl.setValue(0)
        self.mock_tlb.setChecked(0)
        self.mock_gb.setChecked(0)

        self.view.set_single_state(self.mock_sb, 1)
        self.view.set_single_state(self.mock_dsb, 1)
        self.view.set_single_state(self.mock_le, 1)
        self.view.set_single_state(self.mock_cb, 1)
        self.view.set_single_state(self.mock_rb, 1)
        self.view.set_single_state(self.mock_combb, "B")
        self.view.set_single_state(self.mock_sl, 1)
        self.view.set_single_state(self.mock_tlb, 1)
        self.view.set_single_state(self.mock_gb, 1)

        self.assertEqual(self.mock_sb.value(), 1)
        self.assertEqual(self.mock_dsb.value(), 1)
        self.assertEqual(self.mock_le.text(), "1")
        self.assertTrue(self.mock_cb.isChecked())
        self.assertTrue(self.mock_rb.isChecked())
        self.assertEqual(self.mock_combb.currentText(), "B")
        self.assertEqual(self.mock_sl.value(), 1)
        self.assertTrue(self.mock_tlb.isChecked())
        self.assertTrue(self.mock_gb.isChecked())

    def test_get_single_state(self):
        self.mock_sb.setValue(1)
        self.mock_dsb.setValue(1)
        self.mock_le.setText("1")
        self.mock_cb.setChecked(1)
        self.mock_rb.setChecked(1)
        self.mock_combb.setCurrentIndex(1)
        self.mock_sl.setValue(1)
        self.mock_tlb.setChecked(1)
        self.mock_gb.setChecked(1)

        self.assertEqual(self.view._get_single_state(self.mock_sb), 1)
        self.assertEqual(self.view._get_single_state(self.mock_dsb), 1)
        self.assertEqual(self.view._get_single_state(self.mock_le), "1")
        self.assertTrue(self.view._get_single_state(self.mock_cb))
        self.assertTrue(self.view._get_single_state(self.mock_rb))
        self.assertEqual(self.view._get_single_state(self.mock_combb), "B")
        self.assertEqual(self.view._get_single_state(self.mock_sl), 1)
        self.assertTrue(self.view._get_single_state(self.mock_tlb))
        self.assertTrue(self.view._get_single_state(self.mock_gb))
        self.assertIsNone(self.view._get_single_state(1))

    def test_get_state(self):
        testv = self.view.get_state()
        self.assertIsInstance(testv, OrderedDict)
        self.mock_sb.setValue(1)
        self.assertEqual(testv, OrderedDict({"myname": 1}))

    def test_set_state(self):
        self.mock_sb.setValue(0)
        state_dict = OrderedDict({"myname": 1})
        self.view.set_state(state_dict)
        self.assertEqual(self.mock_sb.value(), 1)

    @patch("mantidqtinterfaces.dns_powder_tof.data_structures.dns_view.QMessageBox")
    def test_raise_error(self, mock_messagebox):
        self.view.raise_error("123", critical=False, info=False)
        mock_messagebox.assert_called_once()

    def test_show_status_message(self):
        self.view.parent = mock.Mock()
        self.view.parent.show_status_message = mock.Mock()
        self.view.show_status_message(message="", time=1, clear=False)
        self.view.parent.show_status_message.assert_called_once()


if __name__ == "__main__":
    unittest.main()
