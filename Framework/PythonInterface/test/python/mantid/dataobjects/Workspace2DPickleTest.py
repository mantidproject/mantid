# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
# pylint: disable=invalid-name, too-many-public-methods
import unittest

try:
    import cPickle as pickle
except ImportError:
    import pickle

from mantid.simpleapi import Load, CreateSampleWorkspace


class Workspace2DPickleTest(unittest.TestCase):
    def setUp(self):
        group = Load(Filename="POLREF00004699.nxs", OutputWorkspace="ws_orig")
        self.ws_orig = group[0]
        pickled = pickle.dumps(self.ws_orig, pickle.HIGHEST_PROTOCOL)
        self.ws_copy = pickle.loads(pickled)

    def test_titles_match(self):
        self.assertEqual(self.ws_orig.getTitle(), self.ws_copy.getTitle())

    def test_instrument_match(self):
        self.assertEqual(self.ws_orig.getInstrumentName(), self.ws_copy.getInstrumentName())

    def test_axis_unit_match(self):
        x_orig = self.ws_orig.getAxis(0).getUnit().unitID()
        x_copy = self.ws_copy.getAxis(0).getUnit().unitID()
        y_orig = self.ws_orig.getAxis(1).getUnit().unitID()
        y_copy = self.ws_copy.getAxis(1).getUnit().unitID()
        self.assertEqual(x_orig, x_copy)
        self.assertEqual(y_orig, y_copy)

    def test_data_match(self):
        self.assertTrue(all(self.ws_orig.y(12) == self.ws_copy.y(12)))
        self.assertFalse(all(self.ws_orig.y(12) == self.ws_copy.y(23)))
        self.assertTrue(all(self.ws_orig.e(12) == self.ws_copy.e(12)))
        self.assertFalse(all(self.ws_orig.e(12) == self.ws_copy.e(23)))

    def test_pickling_scanning_workspace_forbidden(self):
        ws = CreateSampleWorkspace(NumBanks=1, NumScanPoints=2)
        with self.assertRaises(ValueError):
            pickle.dumps(ws, pickle.HIGHEST_PROTOCOL)


if __name__ == "__main__":
    unittest.main()
