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

    def test_get_bad_cross_section(self):
        """Test that an error is raised if cross section data is missing"""

        with self.assertRaisesRegex(ValueError, "Found NaN cross-section for Zn with 65 nucleons"):
            # Zn65 is unstable and has no recorded cross section values
            AbinsAlgorithm.get_cross_section(nucleons_number=65, protons_number=30)
