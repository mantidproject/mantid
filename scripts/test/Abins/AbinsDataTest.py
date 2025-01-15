# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
import json
from pathlib import Path
import unittest
from unittest.mock import MagicMock
from tempfile import TemporaryDirectory

from numpy.testing import assert_allclose
from pydantic import ValidationError

import abins
from abins.abinsdata import AbinsData
from abins.input import all_loaders
from abins.test_helpers import assert_atom_almost_equal
from abins.test_helpers import assert_kpoint_almost_equal


class TestAbinsData(unittest.TestCase):
    def setUp(self):
        from mantid.kernel import ConfigService

        self.cache_directory = ConfigService.getString("defaultsave.directory")

        self.mock_ad = MagicMock(spec=abins.AtomsData)
        self.mock_kpd = MagicMock(spec=abins.KpointsData)

    def test_init_typeerror(self):
        with self.assertRaisesRegex(ValidationError, "Input should be an instance of KpointsData"):
            AbinsData(k_points_data=1, atoms_data=self.mock_ad)
        with self.assertRaisesRegex(ValidationError, "Input should be an instance of AtomsData"):
            AbinsData(k_points_data=self.mock_kpd, atoms_data={"key": "value"})

    def test_init_noloader(self):
        with self.assertRaises(ValueError):
            AbinsData.from_calculation_data(
                abins.test_helpers.find_file("squaricn_sum_LoadCASTEP.phonon"),
                ab_initio_program="fake_program",
                cache_directory=self.cache_directory,
            )

    def test_data_content(self):
        abins_data = AbinsData(k_points_data=self.mock_kpd, atoms_data=self.mock_ad)
        self.assertEqual(abins_data.get_kpoints_data(), self.mock_kpd)
        self.assertEqual(abins_data.get_atoms_data(), self.mock_ad)
        self.assertEqual(abins_data.extract(), {"k_points_data": self.mock_kpd.extract(), "atoms_data": self.mock_ad.extract()})


class DummyLoader:
    def __init__(self, *, input_ab_initio_filename, cache_directory):
        self.filename = input_ab_initio_filename
        self.cache_directory = cache_directory

    def get_formatted_data(self):
        return "FORMATTED DATA"


class TestAbinsDataFromCalculation(unittest.TestCase):
    def setUp(self):
        from mantid.kernel import ConfigService

        self.cache_directory = ConfigService.getString("defaultsave.directory")

        all_loaders["DUMMYLOADER"] = DummyLoader

    def tearDown(self):
        del all_loaders["DUMMYLOADER"]

    def test_with_dummy_loader(self):
        data = AbinsData.from_calculation_data("dummy_file.ext", "DummyLoader", cache_directory=self.cache_directory)
        self.assertEqual(data, "FORMATTED DATA")


class TestAbinsDataJSON(unittest.TestCase):
    def test_json_roundtrip(self):
        ref_data_file = abins.test_helpers.find_file("mgo-abinsdata-dump.json")
        with open(ref_data_file, "r") as fp:
            abins_data = AbinsData.from_dict(json.load(fp))

        # Check eigenvectors contain complex numbers where expected
        self.assertTrue(abins_data.get_kpoints_data()[1].atomic_displacements.imag.any())

        with TemporaryDirectory() as td:
            tmp_file = Path(td) / "temp.json"
            with open(tmp_file, "w") as fp:
                json.dump(abins_data.to_dict(), fp)

            with open(tmp_file, "r") as fp:
                roundtrip_data = AbinsData.from_dict(json.load(fp))

        for ref_atom, roundtrip_atom in zip(abins_data.get_atoms_data(), roundtrip_data.get_atoms_data()):
            assert_atom_almost_equal(ref_atom, roundtrip_atom, tester=self)

        ref_kpoints_data = abins_data.get_kpoints_data()
        roundtrip_kpoints_data = roundtrip_data.get_kpoints_data()

        assert_allclose(ref_kpoints_data.unit_cell, roundtrip_kpoints_data.unit_cell)

        for ref_kpt, roundtrip_kpt in zip(ref_kpoints_data, roundtrip_kpoints_data):
            assert_kpoint_almost_equal(ref_kpt, roundtrip_kpt)


if __name__ == "__main__":
    unittest.main()
