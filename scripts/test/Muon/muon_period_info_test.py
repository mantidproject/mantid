# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import unittest

from mantidqt.utils.qt.testing import start_qapplication
from Muon.GUI.Common.muon_period_info import MuonPeriodInfoWidget, CYCLES_NOT_FOUND, DAQ, DWELL, NOT_DAQ_STRING


@start_qapplication
class MuonPeriodInfoWidgetTest(unittest.TestCase):
    def setUp(self):
        self.widget = MuonPeriodInfoWidget()

    def assert_row_values(self, row_num, period_number, name, type, daq_number, frames, total_frames, counts):
        self.assertEqual(period_number, self.widget._table.item(row_num, 0).text())
        self.assertEqual(name, self.widget._table.item(row_num, 1).text())
        self.assertEqual(type, self.widget._table.item(row_num, 2).text())
        self.assertEqual(daq_number, self.widget._table.item(row_num, 3).text())
        self.assertEqual(frames, self.widget._table.item(row_num, 4).text())
        self.assertEqual(total_frames, self.widget._table.item(row_num, 5).text())
        self.assertEqual(counts, self.widget._table.item(row_num, 6).text())

    def test_empty_on_initialize(self):
        self.assertEqual(True, self.widget.is_empty())

    def test_clear(self):
        self.assertEqual(True, self.widget.is_empty())

    def test_add_entry(self):
        self.widget.add_period_to_table("state 1 dwell", DWELL, "50", "1000", "25")
        self.widget.add_period_to_table("state 1", DAQ, "10", "200", "25")
        self.assert_row_values(0, "1", "state 1 dwell", "DWELL", NOT_DAQ_STRING, "50", "1000", NOT_DAQ_STRING)
        self.assert_row_values(1, "2", "state 1", "DAQ", "1", "10", "200", "25")
        self.assertEqual(2, self.widget._num_rows())
        self.assertEqual(False, self.widget.is_empty())

    def test_daq_count_increases_correctly(self):
        self.widget.add_period_to_table("state 1", DAQ, "10", "200", "25")
        self.assertEqual(1, self.widget._daq_count)
        self.widget.add_period_to_table("state 1 dwell", DWELL, "50", "1000", "25")
        self.assertEqual(1, self.widget._daq_count)
        self.widget.add_period_to_table("state 2", DAQ, "10", "200", "25")
        self.widget.add_period_to_table("state 3", DAQ, "10", "200", "25")
        self.assertEqual(3, self.widget._daq_count)

    def test_number_of_sequences_when_empty(self):
        self.widget.number_of_sequences = None
        self.assertEqual(CYCLES_NOT_FOUND, str(self.widget._label.text()))

    def test_set_number_of_sequences(self):
        self.widget.number_of_sequences = "100"
        self.assertEqual("Run contains 100 cycles of periods", str(self.widget._label.text()))


if __name__ == '__main__':
    unittest.main(buffer=False, verbosity=2)
