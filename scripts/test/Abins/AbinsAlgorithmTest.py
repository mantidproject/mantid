# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2024 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +

import unittest

# Import mantid.simpleapi first, otherwise we get circular import
import mantid.simpleapi  # noqa: F401

from abins.abinsalgorithm import AbinsAlgorithm


class AtomsDataTest(unittest.TestCase):
    """Test static methods on AbinsAlgorithm"""

    def test_cross_section(self):
        """Get cross section from nucleus information"""

        for scattering, nucleons_number, protons_number, expected in [
            ("Incoherent", 67, 30, 0.28),
            ("Coherent", None, 30, 4.054),
            ("Total", None, 1, 82.02),
        ]:
            xc = AbinsAlgorithm.get_cross_section(
                scattering=scattering,
                nucleons_number=nucleons_number,
                protons_number=protons_number,
            )

            self.assertAlmostEqual(xc, expected)

    def test_get_bad_cross_section(self):
        """Test that an error is raised if cross section data is missing"""

        with self.assertRaisesRegex(ValueError, "Found NaN cross-section for Zn with 65 nucleons"):
            # Zn65 is unstable and has no recorded cross section values
            AbinsAlgorithm.get_cross_section(nucleons_number=65, protons_number=30)
