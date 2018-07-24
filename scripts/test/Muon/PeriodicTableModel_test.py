from __future__ import absolute_import, print_function

import unittest

from Muon.GUI.ElementalAnalysis.PeriodicTable.periodic_table_model import PeriodicTableModel

try:
    from unittest import mock
except ImportError:
    import mock

class PeriodicTableModelTest(unittest.TestCase):
    def setUp(self):
        self.model = PeriodicTableModel()
        self.model.load_peak_data = mock.Mock()
        #self.model.peak_data = mock.MagicMock()
        #self._peak_data = {"Test": 1243}
        #self.model.peak_data.__getitem__.side_effect = peak_data.__getitem__

    def test_peak_data_file_setter(self):
        self.model.peak_data_file = mock.Mock()
        assert self.model.load_peak_data.call_count == 1

    def test_peak_data_file_getter(self):
        m = mock.Mock()
        self.model.peak_data_file = m
        assert self.model.peak_data_file == m

if __name__ == "__main__":
    unittest.main()