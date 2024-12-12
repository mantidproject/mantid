# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import unittest
from mantid.kernel import V3D, SpecialCoordinateSystem
from mantid.dataobjects import NoShape, PeakShapeSpherical, PeakShapeEllipsoid, PeakShapeDetectorBin
from mantid.simpleapi import CreateSimulationWorkspace, CreatePeaksWorkspace, LoadInstrument
import numpy as np
import numpy.testing as npt
import json


class IPeakTest(unittest.TestCase):
    def setUp(self):
        # IPeak cannot currently be instatiated so this is a quick way
        # getting a handle to a peak object
        ws = CreateSimulationWorkspace("SXD", BinParams="1,1,10")
        # load IDF pre cycle 19_1 when bank 1 upgraded and position was changed
        LoadInstrument(Workspace=ws, Filename="SXD_Definition.xml", RewriteSpectraMap=False)
        peaks = CreatePeaksWorkspace(ws, 1)
        self._peak = peaks.getPeak(0)

        # tolerance for differences in q vectors that a recomputed
        # on every call.
        self._tolerance = 1e-2

    def test_set_run_number(self):
        run_number = 101
        self._peak.setRunNumber(run_number)
        self.assertEqual(self._peak.getRunNumber(), run_number)

    def test_set_monitor_count(self):
        mon_count = 3
        self._peak.setMonitorCount(mon_count)
        self.assertEqual(self._peak.getMonitorCount(), mon_count)

    def test_set_hkl_all_at_once(self):
        H, K, L = 1, 2, 3
        self._peak.setHKL(H, K, L)

        self.assertEqual(self._peak.getH(), H)
        self.assertEqual(self._peak.getK(), K)
        self.assertEqual(self._peak.getL(), L)

    def test_set_hkl_individually(self):
        H, K, L = 1, 2, 3
        self._peak.setH(H)
        self._peak.setK(K)
        self._peak.setL(L)

        self.assertEqual(self._peak.getH(), H)
        self.assertEqual(self._peak.getK(), K)
        self.assertEqual(self._peak.getL(), L)

    def test_set_q_lab_frame(self):
        q_lab = V3D(0, 1, 1)
        self._peak.setQLabFrame(q_lab)

        npt.assert_allclose(self._peak.getQLabFrame(), q_lab, atol=self._tolerance)
        npt.assert_allclose(self._peak.getQSampleFrame(), q_lab, atol=self._tolerance)

    def test_set_q_sample_frame(self):
        q_sample = V3D(0, 1, 1)
        self._peak.setQSampleFrame(q_sample)

        npt.assert_allclose(self._peak.getQSampleFrame(), q_sample, atol=self._tolerance)
        npt.assert_allclose(self._peak.getQLabFrame(), q_sample, atol=self._tolerance)

    def test_set_goniometer_matrix_with_valid_matrix(self):
        angle = np.pi / 4
        rotation = np.array([[np.cos(angle), -np.sin(angle), 0], [np.sin(angle), np.cos(angle), 0], [0, 0, 1]])
        q_sample = V3D(1, 1, 1)

        self._peak.setGoniometerMatrix(rotation)
        self._peak.setQSampleFrame(q_sample)

        q_lab = np.dot(self._peak.getQLabFrame(), rotation)
        npt.assert_allclose(self._peak.getGoniometerMatrix(), rotation)
        npt.assert_allclose(self._peak.getQSampleFrame(), q_sample, atol=self._tolerance)
        npt.assert_allclose(q_lab, q_sample, atol=self._tolerance)

    def test_set_goniometer_matrix_with_singular_matrix(self):
        rotation = np.zeros((3, 3))
        self.assertRaises(ValueError, self._peak.setGoniometerMatrix, rotation)

    def test_set_wavelength(self):
        wavelength = 1.3
        self._peak.setWavelength(wavelength)
        self.assertAlmostEqual(self._peak.getWavelength(), wavelength)

    def test_get_scattering(self):
        expected_scattering_angle = 2.7024  # angle subtended by detector with ID=1
        self.assertAlmostEqual(self._peak.getScattering(), expected_scattering_angle, places=4)

    def test_get_tof(self):
        wavelength = 1.9
        expected_tof = 4112.53
        self._peak.setWavelength(wavelength)
        self.assertAlmostEqual(self._peak.getTOF(), expected_tof, places=2)

    def test_get_d_spacing(self):
        wavelength = 1.9
        expected_d = 0.973
        self._peak.setWavelength(wavelength)
        self.assertAlmostEqual(self._peak.getDSpacing(), expected_d, places=3)

    def test_set_initial_energy(self):
        initial_energy = 10.0
        self._peak.setInitialEnergy(initial_energy)
        self.assertAlmostEqual(self._peak.getInitialEnergy(), initial_energy)

    def test_set_final_energy(self):
        final_energy = 10.0
        self._peak.setFinalEnergy(final_energy)
        self.assertAlmostEqual(self._peak.getFinalEnergy(), final_energy)

    def test_get_energy(self):
        initial_energy = 10.0
        final_energy = 10.0
        self._peak.setFinalEnergy(final_energy)
        self._peak.setInitialEnergy(initial_energy)
        self.assertAlmostEqual(self._peak.getEnergyTransfer(), initial_energy - final_energy)

    def test_set_intensity(self):
        intensity = 10.0
        self._peak.setIntensity(intensity)
        self.assertAlmostEqual(self._peak.getIntensity(), intensity)

    def test_set_sigma_intensity(self):
        sigma = 10.0
        self._peak.setSigmaIntensity(sigma)
        self.assertAlmostEqual(self._peak.getSigmaIntensity(), sigma)

    def test_get_intensity_over_sigma(self):
        intensity = 100.0
        sigma = 10.0
        self._peak.setIntensity(intensity)
        self._peak.setSigmaIntensity(sigma)
        self.assertAlmostEqual(self._peak.getIntensityOverSigma(), intensity / sigma)

    def test_set_bin_count(self):
        bin_count = 10.0
        self._peak.setBinCount(bin_count)
        self.assertAlmostEqual(self._peak.getBinCount(), bin_count)

    def test_get_row_and_column(self):
        row, col = 0, 0  # this is the very first detector
        self.assertEqual(self._peak.getRow(), row)
        self.assertEqual(self._peak.getCol(), col)

    def test_get_l1(self):
        expected_l1 = 8.3
        self.assertEqual(self._peak.getL1(), expected_l1)

    def test_get_l2(self):
        expected_l2 = 0.26279
        self.assertAlmostEqual(self._peak.getL2(), expected_l2, places=4)

    def test_set_modulation_vector(self):
        test_vector = V3D(0.5, 0, 0.2)
        test_vector_out = V3D(1, 0, 0)
        self._peak.setIntMNP(test_vector)
        self.assertEqual(self._peak.getIntMNP(), test_vector_out)

    def test_set_get_inthkl(self):
        test_vector = V3D(0.5, 0, 0.2)
        self._peak.setIntHKL(test_vector)
        self.assertEqual(self._peak.getIntHKL(), V3D(1, 0, 0))

    def test_set_peak_shape(self):
        no_shape = NoShape()
        sphere = PeakShapeSpherical(1)
        ellipse = PeakShapeEllipsoid([V3D(1, 0, 0), V3D(0, 1, 0), V3D(0, 0, 1)], [0.1, 0.2, 0.3], [0.4, 0.5, 0.6], [0.7, 0.8, 0.9])
        detector_x_list = [(14500, 20.45, 50.89), (45670, 109.88, 409.56), (60995, 56009.89, 70988.89)]
        detector_bin = PeakShapeDetectorBin(detector_x_list, SpecialCoordinateSystem.NONE, "test", 1)

        self.assertEqual(self._peak.getPeakShape().shapeName(), "none")

        self._peak.setPeakShape(sphere)
        self.assertEqual(self._peak.getPeakShape().shapeName(), "spherical")

        self._peak.setPeakShape(ellipse)
        self.assertEqual(self._peak.getPeakShape().shapeName(), "ellipsoid")

        self._peak.setPeakShape(no_shape)
        self.assertEqual(self._peak.getPeakShape().shapeName(), "none")

        self._peak.setPeakShape(detector_bin)
        self.assertEqual(self._peak.getPeakShape().shapeName(), "PeakShapeDetectorBin")
        self.assertEqual(self._peak.getPeakShape().algorithmVersion(), 1)
        self.assertEqual(self._peak.getPeakShape().algorithmName(), "test")
        det_bin_dict = json.loads(self._peak.getPeakShape().toJSON())
        for i in range(3):
            self.assertEqual(det_bin_dict["detectors"][i]["detId"], detector_x_list[i][0])
            self.assertEqual(det_bin_dict["detectors"][i]["startX"], detector_x_list[i][1])
            self.assertEqual(det_bin_dict["detectors"][i]["endX"], detector_x_list[i][2])


if __name__ == "__main__":
    unittest.main()
