# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2024 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +

import unittest

# Import mantid.simpleapi first, otherwise we get circular import
import mantid.simpleapi  # noqa: F401

from abins.abinsalgorithm import AbinsAlgorithm, AtomInfo


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


class AtomsDataTest(unittest.TestCase):
    """Test static methods on AbinsAlgorithm"""

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
