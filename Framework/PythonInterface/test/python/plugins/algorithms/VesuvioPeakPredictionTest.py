# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import unittest
from mantid.simpleapi import VesuvioPeakPrediction
from mantid.api import ITableWorkspace


class VesuvioPeakPredictionTest(unittest.TestCase):
    def test_debye(self):
        """
        Test of debye model with multiple temperatures
        """
        vesuvio_debye_params = VesuvioPeakPrediction(
            Model="Debye", Temperature=[100, 120, 240, 300], AtomicMass=63.5, Frequency=20, DebyeTemperature=347
        )

        self.assertTrue(isinstance(vesuvio_debye_params, ITableWorkspace))
        self.assertEqual(vesuvio_debye_params.cell(1, 1), 63.5)
        self.assertEqual(round(vesuvio_debye_params.cell(2, 5), 7), 0.0694458)

    def test_einstein(self):
        """
        Test of einstein model with multiple temperatures
        """
        vesuvio_einstein_params = VesuvioPeakPrediction(
            Model="Einstein", Temperature=[100, 120, 240, 300], AtomicMass=63.5, Frequency=20, DebyeTemperature=347
        )

        self.assertTrue(isinstance(vesuvio_einstein_params, ITableWorkspace))
        self.assertEqual(vesuvio_einstein_params.cell(1, 1), 63.5)
        self.assertEqual(round(vesuvio_einstein_params.cell(2, 5), 5), 18.47381)

    # -------------------------Failure Cases-------------------------

    def test_temperature(self):
        """
        Test negative input temperature
        """
        self.assertRaisesRegex(
            ValueError,
            "Selected value -20 is < the lower bound",
            VesuvioPeakPrediction,
            Model="Debye",
            Temperature=[100, 0, 240, -20],
            AtomicMass=63.5,
            Frequency=20,
            DebyeTemperature=347,
        )

    def test_frequency(self):
        """
        Test zero frequency
        """
        self.assertRaisesRegex(
            ValueError,
            "Selected value 0 is <= the lower bound",
            VesuvioPeakPrediction,
            Model="Einstein",
            Temperature=[100, 0, 240],
            AtomicMass=63.5,
            Frequency=0.0,
            DebyeTemperature=347,
        )

    def test_debye_temp(self):
        """
        Test zero debye temp
        """
        self.assertRaisesRegex(
            ValueError,
            "Selected value -540 is <= the lower bound",
            VesuvioPeakPrediction,
            Model="Debye",
            Temperature=[100, 0, 240],
            AtomicMass=63.5,
            Frequency=20,
            DebyeTemperature=-540,
        )


if __name__ == "__main__":
    unittest.main()
