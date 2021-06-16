# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import unittest

from Muon.GUI.Common.muon_base import MuonBase


class MuonBaseTest(unittest.TestCase):
    """
        The MuonBase object encapsulates the basic information that describes a group or pair:
    """

    def test_cannot_initialize_without_name(self):
        with self.assertRaises(TypeError):
            MuonBase()

    def test_name_is_set_correctly(self):
        muon_base = MuonBase("base")
        self.assertEqual(muon_base.name, "base")

    def test_that_cannot_set_new_name(self):
        muon_base = MuonBase("base")

        with self.assertRaises(AttributeError):
            muon_base.name = "new_name"
        self.assertEqual(muon_base.name, "base")


if __name__ == '__main__':
    unittest.main(buffer=False, verbosity=2)
