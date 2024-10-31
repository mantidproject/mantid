# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import unittest
from mantid.api import AlgorithmManager
from mantid.simpleapi import CreateSampleWorkspace


class DetectorFloodWeightingTest(unittest.TestCase):
    def _create_ws(self, units="TOF", signal_value=2, data_x=range(0, 10), n_spec=1):
        data_y = [signal_value] * (len(data_x) - 1)
        alg = AlgorithmManager.create("CreateWorkspace")
        alg.setChild(True)
        alg.initialize()
        alg.setProperty("DataX", data_x)
        alg.setProperty("DataY", data_y)
        alg.setProperty("NSpec", n_spec)
        alg.setProperty("OutputWorkspace", "temp")
        alg.setProperty("UnitX", units)
        alg.execute()
        return alg.getProperty("OutputWorkspace").value

    def test_init(self):
        alg = AlgorithmManager.create("DetectorFloodWeighting")
        alg.initialize()
        self.assertTrue(alg.isInitialized())

    def test_input_must_be_wavelength(self):
        tof_ws = self._create_ws(units="TOF")
        alg = AlgorithmManager.create("DetectorFloodWeighting")
        alg.setChild(True)
        alg.initialize()
        self.assertRaises(ValueError, alg.setProperty, "InputWorkspace", tof_ws)

    def test_cannot_have_negative_wavlength_boundaries_in_bands(self):
        alg = AlgorithmManager.create("DetectorFloodWeighting")
        alg.setChild(True)
        alg.initialize()
        self.assertRaises(ValueError, alg.setProperty, "Bands", [-1, 10])

    def test_bands_must_not_overlap(self):
        alg = AlgorithmManager.create("DetectorFloodWeighting")
        alg.setChild(True)
        alg.initialize()
        signal_value = 2
        in_ws = self._create_ws(units="Wavelength", signal_value=signal_value, data_x=range(0, 10, 1))
        alg.setProperty("InputWorkspace", in_ws)
        bands = [1, 3, 2, 4]  # Overlap!
        alg.setProperty("Bands", bands)  # One band
        alg.setPropertyValue("OutputWorkspace", "dummy")
        self.assertRaisesRegex(RuntimeError, "Bands must not intersect", alg.execute)

    def test_execute_single_no_solid_angle(self):
        alg = AlgorithmManager.create("DetectorFloodWeighting")
        alg.setChild(True)
        alg.initialize()
        alg.setProperty("SolidAngleCorrection", False)
        signal_value = 2
        in_ws = self._create_ws(units="Wavelength", signal_value=signal_value, data_x=range(0, 10, 1))
        alg.setProperty("InputWorkspace", in_ws)
        bands = [1, 10]
        alg.setProperty("Bands", bands)  # One band
        alg.setPropertyValue("OutputWorkspace", "dummy")
        alg.execute()

        out_ws = alg.getProperty("OutputWorkspace").value
        self.assertEqual(1, out_ws.blocksize())
        self.assertEqual("Wavelength", out_ws.getAxis(0).getUnit().unitID())
        self.assertEqual(in_ws.getNumberHistograms(), out_ws.getNumberHistograms(), msg="Number of histograms should be unchanged.")
        x_axis = out_ws.readX(0)
        self.assertEqual(x_axis[0], bands[0])
        self.assertEqual(x_axis[-1], bands[-1])
        self.assertEqual(out_ws.readY(0)[0], 1.0)

    def test_execute_multiple_bands_no_solid_angle(self):
        alg = AlgorithmManager.create("DetectorFloodWeighting")
        alg.setChild(True)
        alg.initialize()
        alg.setProperty("SolidAngleCorrection", False)
        signal_value = 2
        in_ws = self._create_ws(units="Wavelength", signal_value=signal_value, data_x=range(0, 10, 1))
        alg.setProperty("InputWorkspace", in_ws)
        bands = [1, 2, 3, 4]
        alg.setProperty("Bands", bands)  # One band
        alg.setPropertyValue("OutputWorkspace", "dummy")
        alg.execute()

        out_ws = alg.getProperty("OutputWorkspace").value
        self.assertEqual(1, out_ws.blocksize())
        self.assertEqual("Wavelength", out_ws.getAxis(0).getUnit().unitID())
        self.assertEqual(in_ws.getNumberHistograms(), out_ws.getNumberHistograms(), msg="Number of histograms should be unchanged.")
        x_axis = out_ws.readX(0)
        self.assertEqual(x_axis[0], bands[0])
        self.assertEqual(x_axis[-1], bands[-1])
        self.assertEqual(out_ws.readY(0)[0], 1.0)

    def test_execute_multiple_bands_no_solid_angle_with_transmission(self):
        alg = AlgorithmManager.create("DetectorFloodWeighting")
        alg.setChild(True)
        alg.initialize()
        alg.setProperty("SolidAngleCorrection", False)
        signal_value = 2
        in_ws = self._create_ws(units="Wavelength", signal_value=signal_value, data_x=range(0, 10, 1))
        alg.setProperty("InputWorkspace", in_ws)
        alg.setProperty("TransmissionWorkspace", in_ws)
        bands = [1, 2, 3, 4]
        alg.setProperty("Bands", bands)  # One band
        alg.setPropertyValue("OutputWorkspace", "dummy")
        alg.execute()

        out_ws = alg.getProperty("OutputWorkspace").value
        self.assertEqual(1, out_ws.blocksize())
        self.assertEqual("Wavelength", out_ws.getAxis(0).getUnit().unitID())
        self.assertEqual(in_ws.getNumberHistograms(), out_ws.getNumberHistograms(), msg="Number of histograms should be unchanged.")
        x_axis = out_ws.readX(0)
        self.assertEqual(x_axis[0], bands[0])
        self.assertEqual(x_axis[-1], bands[-1])
        self.assertEqual(out_ws.readY(0)[0], 1.0)

    def test_execute_single_with_solid_angle(self):
        alg = AlgorithmManager.create("DetectorFloodWeighting")
        alg.setChild(True)
        alg.initialize()
        alg.setProperty("SolidAngleCorrection", True)
        signal_value = 2
        in_ws = CreateSampleWorkspace(NumBanks=1, XUnit="Wavelength")
        alg.setProperty("InputWorkspace", in_ws)
        bands = [1, 10]
        alg.setProperty("Bands", bands)  # One band
        alg.setPropertyValue("OutputWorkspace", "dummy")
        alg.execute()

        out_ws = alg.getProperty("OutputWorkspace").value
        self.assertEqual(1, out_ws.blocksize())
        self.assertEqual("Wavelength", out_ws.getAxis(0).getUnit().unitID())
        self.assertEqual(in_ws.getNumberHistograms(), out_ws.getNumberHistograms(), msg="Number of histograms should be unchanged.")


if __name__ == "__main__":
    unittest.main()
