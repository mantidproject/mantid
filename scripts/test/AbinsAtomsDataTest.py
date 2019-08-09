# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
from __future__ import (absolute_import, division, print_function)
import unittest
from mantid.simpleapi import logger
import numpy as np
from AbinsModules import AtomsData


class AbinsAtomsDataTest(unittest.TestCase):
    _good_data = {"atom_0": {'sort': 0, 'symbol': 'Si', 'coord': np.asarray([0.,  0.,  0.]), 'mass': 28.085500},
                  "atom_1": {'sort': 1, 'symbol': 'Si', 'coord': np.asarray([0.25,  0.25,  0.25]), 
                             'mass': 28.085500}}

    def setUp(self):
        self._tester = AtomsData(num_atoms=2)

    # constructor
    def test_wrong_num_atoms(self):
        with self.assertRaises(ValueError):
            # noinspection PyUnusedLocal
            wrong_tester = AtomsData(num_atoms=-2)

    # append
    def test_wrong_sort(self):

        # too large
        wrong_data = {"atom_0": {"sort": 3,
                                 "symbol":  self._good_data["atom_0"]["symbol"],
                                 "coord": self._good_data["atom_0"]["coord"],
                                 "mass": self._good_data["atom_0"]["mass"]
                                 }}
        with self.assertRaises(ValueError):
            self._tester._append(item=wrong_data)

        wrong_data["atom_0"]["sort"] = 2  # we count from zero so 2 is also to large
        with self.assertRaises(ValueError):
            self._tester._append(item=wrong_data)

        # negative
        wrong_data["atom_0"]["sort"] = -1,

        with self.assertRaises(ValueError):
            self._tester._append(item=wrong_data)

        # string instead of number
        wrong_data["atom_0"]["sort"] = "-1"

        with self.assertRaises(ValueError):
            self._tester._append(item=wrong_data)

    def test_wrong_symbol(self):
        wrong_data = {"atom_0": {"sort": self._good_data["atom_0"]["sort"],
                                 "symbol": "CA",  # valid is Ca not CA
                                 "coord": self._good_data["atom_0"]["coord"],
                                 "mass": self._good_data["atom_0"]["mass"]
                                 }}
        with self.assertRaises(ValueError):
            self._tester._append(item=wrong_data)

    def test_wrong_coord(self):
        wrong_data = {"atom_0": {"sort": self._good_data["atom_0"]["sort"],
                                 "symbol": self._good_data["atom_0"]["symbol"],
                                 "coord": "wrong", "mass": self._good_data["atom_0"]["mass"]
                                 }}
        with self.assertRaises(ValueError):
            self._tester._append(item=wrong_data)

        wrong_data["coord"] = np.asarray([[1, 2], [4]])
        with self.assertRaises(ValueError):
            self._tester._append(item=wrong_data)

        wrong_data["coord"] = np.asarray([1, 2])
        with self.assertRaises(ValueError):
            self._tester._append(item=wrong_data)

    def test_wrong_atom(self):
        wrong_data = {"atom_0": {"sort": self._good_data["atom_0"]["sort"],
                                 "symbol": self._good_data["atom_0"]["symbol"],
                                 "coord": self._good_data["atom_0"]["coord"],
                                 "mass": self._good_data["atom_0"]["mass"]}}

        with self.assertRaises(ValueError):
            self._tester._append(item=wrong_data)

        wrong_data["atom"] = 2
        with self.assertRaises(ValueError):
            self._tester._append(item=wrong_data)

        wrong_data["atom"] = 3
        with self.assertRaises(ValueError):
            self._tester._append(item=wrong_data)

    def test_wrong_mass(self):

        wrong_data = {"atom_0": {"sort": self._good_data["atom_0"]["sort"],
                                 "symbol": self._good_data["atom_0"]["symbol"],
                                 "coord": self._good_data["atom_0"]["coord"],
                                 "mass": -1.0
                                 }}
        with self.assertRaises(ValueError):
            self._tester._append(item=wrong_data)

        wrong_data["mass"] = 28  # int instead of float
        with self.assertRaises(ValueError):
            self._tester._append(item=wrong_data)

    # set
    def test_wrong_list(self):

        items = self._good_data["atom_0"]  # dict data for one atom only
        with self.assertRaises(ValueError):
            self._tester.set(items=items)

    def test_wrong_extra_append(self):

        self._tester._append(self._good_data["atom_0"])
        self._tester._append(self._good_data["atom_1"])

        with self.assertRaises(ValueError):
            self._tester._append({'sort': 1, 'symbol': 'Si', 'coord': np.asarray([0.25,  0.25,  0.25]),
                                  'mass': 28.085500})

    def test_wrong_set(self):
        data = {}
        data.update({"atom_1": self._good_data["atom_0"]})
        data.update({"atom_2": self._good_data["atom_1"]})
        with self.assertRaises(ValueError):
            self._tester.set(items=data)

    # extract
    def wrong_size_of_data(self):
        self._tester._append(self._good_data["atom_0"])
        with self.assertRaises(ValueError):
            self._tester.extract()

    # valid situations
    def test_good_append(self):

        self._tester._append(self._good_data["atom_0"])
        self._tester._append(self._good_data["atom_1"])

        data = self._tester.extract()
        elements = len(self._good_data)
        for el in range(elements):
            self.assertEqual(self._good_data["atom_%s" % el]["sort"], data["atom_%s" % el]["sort"])
            self.assertEqual(self._good_data["atom_%s" % el]["symbol"], data["atom_%s" % el]["symbol"])
            self.assertEqual(True, np.allclose(self._good_data["atom_%s" % el]["coord"],
                                               data["atom_%s" % el]["coord"]))
            self.assertEqual(self._good_data["atom_%s" % el]["mass"], data["atom_%s" % el]["mass"])

    def test_good_set(self):
        self._tester.set(self._good_data)
        data = self._tester.extract()
        elements = len(self._good_data)

        for el in range(elements):
            self.assertEqual(self._good_data["atom_%s" % el]["sort"], data["atom_%s" % el]["sort"])
            self.assertEqual(self._good_data["atom_%s" % el]["symbol"], data["atom_%s" % el]["symbol"])
            self.assertEqual(True, np.allclose(self._good_data["atom_%s" % el]["coord"],
                                               data["atom_%s" % el]["coord"]))
            self.assertEqual(self._good_data["atom_%s" % el]["mass"], data["atom_%s" % el]["mass"])

if __name__ == '__main__':
    unittest.main()
