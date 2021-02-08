# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import unittest

from mantid.kernel import PhysicalConstants as pc


class PhysicalConstantsTest(unittest.TestCase):
    def test_constants_are_imported_correctly(self):
        self.assertNotEqual(0, pc.N_A)
        self.assertNotEqual(0, pc.h)
        self.assertNotEqual(0, pc.h_bar)
        self.assertNotEqual(0, pc.g)
        self.assertNotEqual(0, pc.NeutronMass)
        self.assertNotEqual(0, pc.NeutronMassAMU)
        self.assertNotEqual(0, pc.AtomicMassUnit)
        self.assertNotEqual(0, pc.meV)
        self.assertNotEqual(0, pc.meVtoWavenumber)
        self.assertNotEqual(0, pc.meVtoFrequency)
        self.assertNotEqual(0, pc.meVtoKelvin)
        self.assertNotEqual(0, pc.E_mev_toNeutronWavenumberSq)
        self.assertNotEqual(0, pc.MuonLifetime)
        self.assertNotEqual(0, pc.StandardAtmosphere)
        self.assertNotEqual(0, pc.BoltzmannConstant)
        self.assertNotEqual(0, pc.MuonGyromagneticRatio)


if __name__ == '__main__':
    unittest.main()
