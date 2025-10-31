# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2025 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +

from tempfile import TemporaryDirectory
import unittest

from mantid.simpleapi import mtd, Abins2D


class Abins2DBasicTest(unittest.TestCase):
    base_kwargs = {
        "VibrationalOrPhononFile": "squaricn_sum_Abins.phonon",
        "AbInitioProgram": "CASTEP",
        "OutputWorkspace": "output_workspace",
        "TemperatureInKelvin": 10.0,
        "Atoms": "",
        "SumContributions": False,
        "SaveAscii": False,
        "ScaleByCrossSection": "Incoherent",
        "QuantumOrderEventsNumber": "1",
        "Autoconvolution": False,
        "EnergyUnits": "meV",
        "Instrument": "Ideal2D",
        "Chopper": "",
        "IncidentEnergy": "500",
        "ChopperFrequency": "300",
    }

    _tolerance = 0.0001

    def setUp(self):
        self.tempdir = TemporaryDirectory()
        self._cache_directory = self.tempdir.name

    def tearDown(self):
        self.tempdir.cleanup()
        mtd.clear()

    def test_basic(self):
        """Check that Abins2D algorithm runs without error using basic parameters"""

        Abins2D(**(self.base_kwargs | {"CacheDirectory": self._cache_directory}))

    def test_wrong_input(self):
        """Check that out-of-range incident energy creates error running algorithm

        A more detailed set of cases is considered in unit-tests of the
        abinsalgorithm.validate_e_init function"""
        with self.assertRaisesRegex(
            RuntimeError,
            "Incident energy cannot be greater than 1000.000 meV for this instrument.",
        ):
            Abins2D(**(self.base_kwargs | {"CacheDirectory": self._cache_directory, "Instrument": "MARI", "IncidentEnergy": "1001"}))


if __name__ == "__main__":
    unittest.main()
