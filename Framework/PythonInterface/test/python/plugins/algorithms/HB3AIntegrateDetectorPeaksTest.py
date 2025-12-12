# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import unittest
import numpy as np
from mantid.simpleapi import HB3AIntegrateDetectorPeaks, HB3AAdjustSampleNorm, DeleteWorkspace, mtd


class HB3ADetectorPeaksTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        HB3AAdjustSampleNorm("HB3A_data.nxs", OutputType="Detector", NormaliseBy="None", OutputWorkspace="data")

    @classmethod
    def tearDownClass(cls):
        DeleteWorkspace("data")

    def testIntegratePeaksFitted(self):
        data = mtd["data"]
        # ChiSq is larger than maximum
        peaks = HB3AIntegrateDetectorPeaks(data, ChiSqMax=10, ApplyLorentz=False, OptimizeQVector=False)
        self.assertEqual(peaks.getNumberPeaks(), 0)

        # Signal/Noise ratio is lower than requested
        peaks = HB3AIntegrateDetectorPeaks(data, ChiSqMax=100, SignalNoiseMin=100, ApplyLorentz=False, OptimizeQVector=False)
        self.assertEqual(peaks.getNumberPeaks(), 0)

        # Actually fit peak in this one
        peaks = HB3AIntegrateDetectorPeaks(data, ChiSqMax=100, ApplyLorentz=False, OptimizeQVector=False)
        self.assertEqual(peaks.getNumberPeaks(), 1)

        peak0 = peaks.getPeak(0)
        self.assertAlmostEqual(peak0.getH(), 0, places=1)
        self.assertAlmostEqual(peak0.getK(), 0, places=1)
        self.assertAlmostEqual(peak0.getL(), 6, places=1)
        self.assertAlmostEqual(peak0.getIntensity(), 961.61533, delta=1e-5)
        self.assertAlmostEqual(peak0.getSigmaIntensity(), 10.479567, delta=3e-2)
        self.assertAlmostEqual(peak0.getWavelength(), 1.008)
        self.assertAlmostEqual(peak0.getAzimuthal(), -np.pi, delta=2e-5)
        self.assertAlmostEqual(peak0.getScattering(), np.deg2rad(data.getExperimentInfo(0).run()["2theta"].value[0]), delta=1e-5)

        q_sample = peak0.getQSampleFrame()
        expected_q_sample = data.getExperimentInfo(0).sample().getOrientedLattice().qFromHKL(peak0.getHKL())
        for i in range(3):
            self.assertAlmostEqual(q_sample[i], expected_q_sample[i])

        # Try with larger ROI, should get slightly different intensity
        peaks = HB3AIntegrateDetectorPeaks(
            data, ChiSqMax=100, LowerLeft=[0, 0], UpperRight=[512, 512], ApplyLorentz=False, OptimizeQVector=False
        )
        self.assertEqual(peaks.getNumberPeaks(), 1)

        peak0 = peaks.getPeak(0)
        self.assertAlmostEqual(peak0.getH(), 0, places=1)
        self.assertAlmostEqual(peak0.getK(), 0, places=1)
        self.assertAlmostEqual(peak0.getL(), 6, places=1)
        self.assertAlmostEqual(peak0.getIntensity(), 990.10107, delta=1e-5)
        self.assertAlmostEqual(peak0.getSigmaIntensity(), 11.70648, delta=1e-5)
        self.assertAlmostEqual(peak0.getWavelength(), 1.008)
        self.assertAlmostEqual(peak0.getAzimuthal(), -np.pi, delta=2e-5)
        self.assertAlmostEqual(peak0.getScattering(), np.deg2rad(data.getExperimentInfo(0).run()["2theta"].value[0]), delta=1e-5)

        q_sample = peak0.getQSampleFrame()
        for i in range(3):
            self.assertAlmostEqual(q_sample[i], expected_q_sample[i])

        # Lorentz correction should scale the intensity by `sin(2theta)`
        peaks = HB3AIntegrateDetectorPeaks(data, ChiSqMax=100, ApplyLorentz=True, OptimizeQVector=False)
        self.assertEqual(peaks.getNumberPeaks(), 1)

        peak0 = peaks.getPeak(0)
        self.assertAlmostEqual(peak0.getScattering(), np.deg2rad(data.getExperimentInfo(0).run()["2theta"].value[0]), delta=1e-5)
        self.assertAlmostEqual(
            peak0.getIntensity(), 961.6164 * np.sin(np.deg2rad(data.getExperimentInfo(0).run()["2theta"].value[0])), delta=1e-2
        )
        self.assertAlmostEqual(
            peak0.getSigmaIntensity(), 10.479567 * np.sin(np.deg2rad(data.getExperimentInfo(0).run()["2theta"].value[0])), delta=2e-2
        )

        # Optimize Q vector, will change Q-sample but the integration
        # should be the same except the Lorentz correction since the
        # scattering angle is changed
        peaks = HB3AIntegrateDetectorPeaks(data, ChiSqMax=100, ApplyLorentz=True, OptimizeQVector=True)
        self.assertEqual(peaks.getNumberPeaks(), 1)

        peak0 = peaks.getPeak(0)
        two_th = peak0.getScattering()
        az = peak0.getAzimuthal()
        nu = np.arcsin(np.sin(two_th) * np.sin(az))
        gamma = np.arctan(np.tan(two_th) * np.cos(az))
        # G. J. McIntyre and R. F. D. Stansfield, Acta Cryst A 44, 257 (1988).
        lorentz = abs(np.cos(nu) * np.sin(gamma))
        self.assertAlmostEqual(peak0.getIntensity(), 961.615331329 * lorentz, delta=1e-5)
        self.assertAlmostEqual(peak0.getSigmaIntensity(), 10.479567046232615 * lorentz, delta=2e-2)
        q_sample = peak0.getQSampleFrame()
        for i in range(3):
            self.assertNotAlmostEqual(q_sample[i], expected_q_sample[i])

        # Try StartX and EndX, should just change the peak intensity slightly
        peaks = HB3AIntegrateDetectorPeaks(data, ChiSqMax=100, ApplyLorentz=False, OptimizeQVector=False, StartX=12.6, EndX=100)
        self.assertEqual(peaks.getNumberPeaks(), 1)
        peak0 = peaks.getPeak(0)
        self.assertAlmostEqual(peak0.getH(), 0, places=1)
        self.assertAlmostEqual(peak0.getK(), 0, places=1)
        self.assertAlmostEqual(peak0.getL(), 6, places=1)
        self.assertAlmostEqual(peak0.getIntensity(), 960.97711, delta=1e-5)
        self.assertAlmostEqual(peak0.getSigmaIntensity(), 10.62189, delta=1e-5)

        DeleteWorkspace(peaks)

    def testIntegratePeaksCounts(self):
        data = mtd["data"]
        peaks = HB3AIntegrateDetectorPeaks(data, Method="Counts", ApplyLorentz=False, OptimizeQVector=False)
        self.assertEqual(peaks.getNumberPeaks(), 1)

        peak0 = peaks.getPeak(0)
        self.assertAlmostEqual(peak0.getH(), 0, places=1)
        self.assertAlmostEqual(peak0.getK(), 0, places=1)
        self.assertAlmostEqual(peak0.getL(), 6, places=1)
        self.assertAlmostEqual(peak0.getIntensity(), 932.24967, delta=1e-5)
        self.assertAlmostEqual(peak0.getSigmaIntensity(), 29.10343, delta=3e-2)
        self.assertAlmostEqual(peak0.getWavelength(), 1.008)
        self.assertAlmostEqual(peak0.getAzimuthal(), -np.pi, delta=2e-5)
        self.assertAlmostEqual(peak0.getScattering(), np.deg2rad(data.getExperimentInfo(0).run()["2theta"].value[0]), delta=1e-5)

        q_sample = peak0.getQSampleFrame()
        expected_q_sample = data.getExperimentInfo(0).sample().getOrientedLattice().qFromHKL(peak0.getHKL())
        for i in range(3):
            self.assertAlmostEqual(q_sample[i], expected_q_sample[i])

        # Lorentz correction should scale the intensity by `sin(2theta)`
        peaks = HB3AIntegrateDetectorPeaks(data, Method="Counts", ApplyLorentz=True, OptimizeQVector=False)
        self.assertEqual(peaks.getNumberPeaks(), 1)

        peak0 = peaks.getPeak(0)
        self.assertAlmostEqual(peak0.getScattering(), np.deg2rad(data.getExperimentInfo(0).run()["2theta"].value[0]), delta=1e-5)
        self.assertAlmostEqual(
            peak0.getIntensity(), 932.24967 * np.sin(np.deg2rad(data.getExperimentInfo(0).run()["2theta"].value[0])), delta=1e-2
        )
        self.assertAlmostEqual(
            peak0.getSigmaIntensity(), 29.10343 * np.sin(np.deg2rad(data.getExperimentInfo(0).run()["2theta"].value[0])), delta=2e-2
        )

        peaks = HB3AIntegrateDetectorPeaks(data, Method="Counts", ApplyLorentz=True, OptimizeQVector=True)
        self.assertEqual(peaks.getNumberPeaks(), 1)

        peak0 = peaks.getPeak(0)
        lorentz = abs(np.cos(peak0.getAzimuthal()) * np.sin(peak0.getScattering()))
        self.assertAlmostEqual(peak0.getIntensity(), 932.24967 * lorentz, delta=1e-2)
        self.assertAlmostEqual(peak0.getSigmaIntensity(), 29.10343 * lorentz, delta=2e-2)
        q_sample = peak0.getQSampleFrame()
        for i in range(3):
            self.assertNotAlmostEqual(q_sample[i], expected_q_sample[i])

        DeleteWorkspace(peaks)

    def testIntegratePeaksCountsWithFitting(self):
        data = mtd["data"]
        peaks = HB3AIntegrateDetectorPeaks(data, Method="CountsWithFitting", ApplyLorentz=False, OptimizeQVector=False, ChiSqMax=100)
        self.assertEqual(peaks.getNumberPeaks(), 1)

        peak0 = peaks.getPeak(0)
        self.assertAlmostEqual(peak0.getH(), 0, places=1)
        self.assertAlmostEqual(peak0.getK(), 0, places=1)
        self.assertAlmostEqual(peak0.getL(), 6, places=1)
        self.assertAlmostEqual(peak0.getIntensity(), 969.77797, delta=1e-5)
        self.assertAlmostEqual(peak0.getSigmaIntensity(), 24.776354, delta=3e-2)
        self.assertAlmostEqual(peak0.getWavelength(), 1.008)
        self.assertAlmostEqual(peak0.getAzimuthal(), -np.pi, delta=2e-5)
        self.assertAlmostEqual(peak0.getScattering(), np.deg2rad(data.getExperimentInfo(0).run()["2theta"].value[0]), delta=1e-5)

        q_sample = peak0.getQSampleFrame()
        expected_q_sample = data.getExperimentInfo(0).sample().getOrientedLattice().qFromHKL(peak0.getHKL())
        for i in range(3):
            self.assertAlmostEqual(q_sample[i], expected_q_sample[i])

        # Lorentz correction should scale the intensity by `sin(2theta)`
        peaks = HB3AIntegrateDetectorPeaks(data, Method="CountsWithFitting", ApplyLorentz=True, OptimizeQVector=False, ChiSqMax=100)
        self.assertEqual(peaks.getNumberPeaks(), 1)

        peak0 = peaks.getPeak(0)
        self.assertAlmostEqual(peak0.getScattering(), np.deg2rad(data.getExperimentInfo(0).run()["2theta"].value[0]), delta=1e-5)
        self.assertAlmostEqual(
            peak0.getIntensity(), 969.778546 * np.sin(np.deg2rad(data.getExperimentInfo(0).run()["2theta"].value[0])), delta=1e-2
        )
        self.assertAlmostEqual(
            peak0.getSigmaIntensity(), 24.776354 * np.sin(np.deg2rad(data.getExperimentInfo(0).run()["2theta"].value[0])), delta=2e-2
        )

        peaks = HB3AIntegrateDetectorPeaks(data, Method="CountsWithFitting", ApplyLorentz=True, OptimizeQVector=True, ChiSqMax=100)
        self.assertEqual(peaks.getNumberPeaks(), 1)

        peak0 = peaks.getPeak(0)
        lorentz = abs(np.cos(peak0.getAzimuthal()) * np.sin(peak0.getScattering()))
        self.assertAlmostEqual(peak0.getIntensity(), 969.778546 * lorentz, delta=1e-2)
        self.assertAlmostEqual(peak0.getSigmaIntensity(), 24.776354 * lorentz, delta=2e-2)
        q_sample = peak0.getQSampleFrame()
        for i in range(3):
            self.assertNotAlmostEqual(q_sample[i], expected_q_sample[i])

        DeleteWorkspace(peaks)


if __name__ == "__main__":
    unittest.main()
