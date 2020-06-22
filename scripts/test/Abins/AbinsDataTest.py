# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
import unittest
from unittest.mock import MagicMock

import abins
from abins.abinsdata import AbinsData


class TestAbinsData(unittest.TestCase):
    def setUp(self):
        self.mock_ad = MagicMock(spec=abins.AtomsData)
        self.mock_kpd = MagicMock(spec=abins.KpointsData)

    def test_init_typeerror(self):
        with self.assertRaises(TypeError):
            AbinsData(k_points_data=1, atoms_data=self.mock_ad)
        with self.assertRaises(TypeError):
            AbinsData(k_points_data=self.mock_kpd, atoms_data={'key': 'value'})

    def test_data_content(self):
        abins_data = AbinsData(k_points_data=self.mock_kpd,
                               atoms_data=self.mock_ad)
        self.assertEqual(abins_data.get_kpoints_data(), self.mock_kpd)
        self.assertEqual(abins_data.get_atoms_data(), self.mock_ad)
        self.assertEqual(abins_data.extract(),
                         {'k_points_data': self.mock_kpd.extract(),
                          'atoms_data': self.mock_ad.extract()})


if __name__ == '__main__':
    unittest.main()
