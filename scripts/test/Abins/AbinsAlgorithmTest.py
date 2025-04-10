# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2024 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +

import unittest

import numpy as np

# Import mantid.simpleapi first, otherwise we get circular import
import mantid.simpleapi  # noqa: F401

from abins.abinsalgorithm import AbinsAlgorithm, AtomInfo
from abins.atomsdata import AtomsData


class AtomInfoTest(unittest.TestCase):
    """Test the AtomInfo class"""

    def test_atom_info(self):
        for args, expected in [
            # Non-standard isotope
            (("Zn", 67.0), {"nucleons_number": 67, "name": "67Zn", "z_number": 30}),
            # Round to standard mix
            (("Zn", 65.4), {"nucleons_number": 0, "name": "Zn", "z_number": 30}),
            # Choose standard mix when isotope with same mass is available
            (("F", 19.0), {"nucleons_number": 0, "name": "F", "z_number": 9}),
        ]:
            atom_info = AtomInfo(*args)
            for attr, value in expected.items():
                self.assertEqual(getattr(atom_info, attr), value)

    def test_bad_atom_info(self):
        """Test that an error is raised if cross section data is missing"""

        # Zn65 is unstable and has no recorded cross section values
        species = AtomInfo(symbol="Zn", mass=65.0)

        with self.assertRaisesRegex(ValueError, "Could not find suitable isotope data for Zn with mass 65.0."):
            species.neutron_data


class AbinsAlgorithmMethodsTest(unittest.TestCase):
    """Test static methods on AbinsAlgorithm"""

    _good_data = {
        "atom_0": {"sort": 0, "symbol": "Si", "coord": np.asarray([0.0, 0.0, 0.0]), "mass": 28.085500},
        "atom_1": {"sort": 1, "symbol": "C", "coord": np.asarray([0.25, 0.25, 0.25]), "mass": 12.0},
        "atom_2": {"sort": 2, "symbol": "C", "coord": np.asarray([0.5, 0.5, 0.5]), "mass": 12.0},
    }

    def setUp(self):
        self._atoms_data = AtomsData(self._good_data)

    def test_cross_section(self):
        """Get cross section from nucleus information"""

        for scattering, nucleons_number, symbol, expected in [
            ("Incoherent", 67, "Zn", 0.28),
            ("Coherent", 0, "Zn", 4.054),
            ("Total", 0, "H", 82.02),
        ]:
            xc = AbinsAlgorithm.get_cross_section(
                scattering=scattering,
                species=AtomInfo(mass=float(nucleons_number), symbol=symbol),
            )

            self.assertAlmostEqual(xc, expected)

    def test_get_atom_selection(self):
        """Get selected symbols and idices from pre-split user input"""

        for selection, expected in [
            (["atom_1", "Si"], ([1], ["Si"])),
            (["atom2", "1"], ([1, 2], [])),
            (["C"], ([], ["C"])),
            (["1-3"], ([1, 2, 3], [])),
            (["3-1"], ([1, 2, 3], [])),
            (["1..2", "C"], ([1, 2], ["C"])),
        ]:
            self.assertEqual(
                AbinsAlgorithm.get_atom_selection(atoms_data=self._atoms_data, selection=selection),
                expected,
            )

        for selection, error_match in [
            (["atom1", "1"], "contains repeated atom"),
            (["Na"], "not present in the system"),
            (["Si", "1", "Si"], "contains repeated species"),
            (["Si123"], r"Not all user atom selections \('atoms' option\) were understood."),
            (["atom3.4"], r"Not all user atom selections \('atoms' option\) were understood."),
        ]:
            with self.assertRaisesRegex(ValueError, error_match):
                AbinsAlgorithm.get_atom_selection(atoms_data=self._atoms_data, selection=selection)
