# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import unittest
import numpy as np
from abins import AtomsData


class AtomsDataTest(unittest.TestCase):
    _good_data = {"atom_0": {'sort': 0, 'symbol': 'Si', 'coord': np.asarray([0.,  0.,  0.]), 'mass': 28.085500},
                  "atom_1": {'sort': 1, 'symbol': 'Si', 'coord': np.asarray([0.25,  0.25,  0.25]),
                             'mass': 28.085500}}

    def setUp(self):
        self._tester = AtomsData(self._good_data)

    def test_direct_access(self):
        self.assertEqual(1, self._tester[1]['sort'])
        self.assertEqual([self._good_data["atom_0"],
                          self._good_data["atom_1"]],
                         self._tester[0:])

    def test_len(self):
        self.assertEqual(len(self._tester), len(self._good_data))

    def test_wrong_sort(self):
        # too large
        wrong_data = {"atom_0": {"sort": 2,
                                 "symbol":  self._good_data["atom_0"]["symbol"],
                                 "coord": self._good_data["atom_0"]["coord"],
                                 "mass": self._good_data["atom_0"]["mass"]
                                 }}

        with self.assertRaises(ValueError):
            AtomsData._check_item(wrong_data["atom_0"], n_atoms=1)

        # negative
        wrong_data["atom_0"]["sort"] = -1,

        with self.assertRaises(ValueError):
            AtomsData._check_item(wrong_data["atom_0"])

        # string instead of number
        wrong_data["atom_0"]["sort"] = "-1"

        with self.assertRaises(ValueError):
            AtomsData._check_item(wrong_data["atom_0"])

    def test_wrong_symbol(self):
        wrong_data = {"atom_0": {"sort": self._good_data["atom_0"]["sort"],
                                 "symbol": "CA",  # valid is Ca not CA
                                 "coord": self._good_data["atom_0"]["coord"],
                                 "mass": self._good_data["atom_0"]["mass"]
                                 }}
        with self.assertRaises(ValueError):
            AtomsData._check_item(wrong_data)

    def test_wrong_coord(self):
        wrong_data = {"atom_0": {"sort": self._good_data["atom_0"]["sort"],
                                 "symbol": self._good_data["atom_0"]["symbol"],
                                 "coord": "wrong", "mass": self._good_data["atom_0"]["mass"]
                                 }}
        with self.assertRaises(ValueError):
            AtomsData._check_item(wrong_data)

        wrong_data["coord"] = np.asarray([[1, 2], [4]])
        with self.assertRaises(ValueError):
            AtomsData._check_item(wrong_data)

        wrong_data["coord"] = np.asarray([1, 2])
        with self.assertRaises(ValueError):
            AtomsData._check_item(wrong_data)

    def test_wrong_atom(self):
        wrong_data = {"atom_0": {"sort": self._good_data["atom_0"]["sort"],
                                 "symbol": self._good_data["atom_0"]["symbol"],
                                 "coord": self._good_data["atom_0"]["coord"],
                                 "mass": self._good_data["atom_0"]["mass"]}}

        with self.assertRaises(ValueError):
            AtomsData._check_item(wrong_data)

        wrong_data["atom"] = 2
        with self.assertRaises(ValueError):
            AtomsData._check_item(wrong_data)

        wrong_data["atom"] = 3
        with self.assertRaises(ValueError):
            AtomsData._check_item(wrong_data)

    def test_wrong_mass(self):
        wrong_data = {"atom_0": {"sort": self._good_data["atom_0"]["sort"],
                                 "symbol": self._good_data["atom_0"]["symbol"],
                                 "coord": self._good_data["atom_0"]["coord"],
                                 "mass": -1.0
                                 }}
        with self.assertRaises(ValueError):
            AtomsData._check_item(wrong_data)

        wrong_data["mass"] = 28  # int instead of float
        with self.assertRaises(ValueError):
            AtomsData._check_item(wrong_data)

    def test_wrong_list(self):
        # init while missing atom 0
        with self.assertRaises(ValueError):
            AtomsData({'atom_1': self._good_data["atom_1"]})

    def test_wrong_set(self):
        data = {"atom_1": self._good_data["atom_0"],
                "atom_2": self._good_data["atom_1"]}
        with self.assertRaises(ValueError):
            AtomsData(data)

    def test_good_extract(self):
        atoms_data = AtomsData(self._good_data)
        data = atoms_data.extract()

        elements = len(self._good_data)
        for el in range(elements):
            self.assertEqual(self._good_data["atom_%s" % el]["sort"], data["atom_%s" % el]["sort"])
            self.assertEqual(self._good_data["atom_%s" % el]["symbol"], data["atom_%s" % el]["symbol"])
            self.assertEqual(True, np.allclose(self._good_data["atom_%s" % el]["coord"],
                                               data["atom_%s" % el]["coord"]))
            self.assertEqual(self._good_data["atom_%s" % el]["mass"], data["atom_%s" % el]["mass"])


if __name__ == '__main__':
    unittest.main()
