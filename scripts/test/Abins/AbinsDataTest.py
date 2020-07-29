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
from abins.input import all_loaders


class TestAbinsData(unittest.TestCase):
    def setUp(self):
        self.mock_ad = MagicMock(spec=abins.AtomsData)
        self.mock_kpd = MagicMock(spec=abins.KpointsData)

    def test_init_typeerror(self):
        with self.assertRaises(TypeError):
            AbinsData(k_points_data=1, atoms_data=self.mock_ad)
        with self.assertRaises(TypeError):
            AbinsData(k_points_data=self.mock_kpd, atoms_data={'key': 'value'})

    def test_init_noloader(self):
        with self.assertRaises(ValueError):
            AbinsData.from_calculation_data(
                abins.test_helpers.find_file("squaricn_sum_LoadCASTEP.phonon"),
                ab_initio_program='fake_program')

    def test_data_content(self):
        abins_data = AbinsData(k_points_data=self.mock_kpd,
                               atoms_data=self.mock_ad)
        self.assertEqual(abins_data.get_kpoints_data(), self.mock_kpd)
        self.assertEqual(abins_data.get_atoms_data(), self.mock_ad)
        self.assertEqual(abins_data.extract(),
                         {'k_points_data': self.mock_kpd.extract(),
                          'atoms_data': self.mock_ad.extract()})


class DummyLoader:
    def __init__(self, *, input_ab_initio_filename):
        self.filename = input_ab_initio_filename

    def get_formatted_data(self):
        return 'FORMATTED DATA'


class TestAbinsDataFromCalculation(unittest.TestCase):
    def setUp(self):
        all_loaders['DUMMYLOADER'] = DummyLoader

    def tearDown(self):
        del all_loaders['DUMMYLOADER']

    def test_with_dummy_loader(self):
        data = AbinsData.from_calculation_data('dummy_file.ext', 'DummyLoader')
        self.assertEqual(data, 'FORMATTED DATA')


if __name__ == '__main__':
    unittest.main()
