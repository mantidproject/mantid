# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import unittest
import numpy as np
from mantid.simpleapi import HFIRCalculateGoniometer, CreatePeaksWorkspace, AddSampleLog, SetGoniometer
from mantid.geometry import Goniometer


class HFIRCalculateGoniometerTest(unittest.TestCase):

    def test_HFIRCalculateGoniometer_HB2C_omega(self):
        omega = np.deg2rad(42)

        R = np.array([[np.cos(omega), 0, np.sin(omega)],
                      [0, 1, 0],
                      [-np.sin(omega), 0, np.cos(omega)]])

        wl = 1.54
        k = 2*np.pi/wl
        theta = np.deg2rad(47)
        phi = np.deg2rad(13)

        q_lab = np.array([-np.sin(theta)*np.cos(phi),
                          -np.sin(theta)*np.sin(phi),
                          1 - np.cos(theta)]) * k

        q_sample = np.dot(np.linalg.inv(R), q_lab)

        peaks = CreatePeaksWorkspace(OutputType="LeanElasticPeak", NumberOfPeaks=0)

        p = peaks.createPeakQSample(q_sample)
        peaks.addPeak(p)

        HFIRCalculateGoniometer(peaks, wl)

        g = Goniometer()
        g.setR(peaks.getPeak(0).getGoniometerMatrix())
        YZY = g.getEulerAngles('YZY')
        self.assertAlmostEqual(YZY[0], 42, delta=1e-10) # omega
        self.assertAlmostEqual(YZY[1], 0, delta=1e-10) # chi
        self.assertAlmostEqual(YZY[2], 0, delta=1e-10) # phi

        self.assertAlmostEqual(peaks.getPeak(0).getWavelength(), 1.54, delta=1e-10)

    def test_HFIRCalculateGoniometer_HB3A_omega(self):
        omega = np.deg2rad(42)
        chi = np.deg2rad(-3)
        phi = np.deg2rad(23)
        R1 = np.array([[np.cos(omega), 0, -np.sin(omega)], # omega 0,1,0,-1
                       [               0, 1,                 0],
                       [np.sin(omega), 0,  np.cos(omega)]])
        R2 = np.array([[ np.cos(chi),  np.sin(chi), 0], # chi 0,0,1,-1
                       [-np.sin(chi),  np.cos(chi), 0],
                       [              0,               0, 1]])
        R3 = np.array([[np.cos(phi), 0, -np.sin(phi)], # phi 0,1,0,-1
                       [             0, 1,               0],
                       [np.sin(phi), 0,  np.cos(phi)]])
        R = np.dot(np.dot(R1, R2), R3)

        wl = 1.54
        k = 2*np.pi/wl
        theta = np.deg2rad(47)
        phi = np.deg2rad(13)

        q_lab = np.array([-np.sin(theta)*np.cos(phi),
                          -np.sin(theta)*np.sin(phi),
                          1 - np.cos(theta)]) * k

        q_sample = np.dot(np.linalg.inv(R), q_lab)

        peaks = CreatePeaksWorkspace(OutputType="LeanElasticPeak", NumberOfPeaks=0)

        p = peaks.createPeakQSample(q_sample)
        peaks.addPeak(p)

        SetGoniometer(Workspace=peaks, Axis0='-3,0,0,1,-1', Axis1='23,0,1,0,-1') # don't set omega

        HFIRCalculateGoniometer(peaks, wl)

        g = Goniometer()
        g.setR(peaks.getPeak(0).getGoniometerMatrix())
        YZY = g.getEulerAngles('YZY')
        self.assertAlmostEqual(YZY[0], -42, delta=1e-10) # omega
        self.assertAlmostEqual(YZY[1], 3, delta=1e-10) # chi
        self.assertAlmostEqual(YZY[2], -23, delta=1e-10) # phi

        self.assertAlmostEqual(peaks.getPeak(0).getWavelength(), 1.54, delta=1e-10)

    def test_HFIRCalculateGoniometer_HB3A_phi(self):
        omega = np.deg2rad(42)
        chi = np.deg2rad(-3)
        phi = np.deg2rad(23)
        R1 = np.array([[np.cos(omega), 0, -np.sin(omega)], # omega 0,1,0,-1
                       [               0, 1,                 0],
                       [np.sin(omega), 0,  np.cos(omega)]])
        R2 = np.array([[ np.cos(chi),  np.sin(chi), 0], # chi 0,0,1,-1
                       [-np.sin(chi),  np.cos(chi), 0],
                       [              0,               0, 1]])
        R3 = np.array([[np.cos(phi), 0, -np.sin(phi)], # phi 0,1,0,-1
                       [             0, 1,               0],
                       [np.sin(phi), 0,  np.cos(phi)]])
        R = np.dot(np.dot(R1, R2), R3)

        wl = 1.54
        k = 2*np.pi/wl
        theta = np.deg2rad(47)
        phi = np.deg2rad(13)

        q_lab = np.array([-np.sin(theta)*np.cos(phi),
                          -np.sin(theta)*np.sin(phi),
                          1 - np.cos(theta)]) * k

        q_sample = np.dot(np.linalg.inv(R), q_lab)

        peaks = CreatePeaksWorkspace(OutputType="LeanElasticPeak", NumberOfPeaks=0)
        AddSampleLog(peaks, "Wavelength", str(wl), "Number")
        SetGoniometer(peaks, Axis0='42,0,1,0,-1', Axis1='-3,0,0,1,-1') # don't set phi

        p = peaks.createPeakQSample(q_sample)
        peaks.addPeak(p)

        HFIRCalculateGoniometer(peaks, OverrideProperty=True, InnerGoniometer=True)

        g = Goniometer()
        g.setR(peaks.getPeak(0).getGoniometerMatrix())
        YZY = g.getEulerAngles('YZY')
        self.assertAlmostEqual(YZY[0], -42, delta=1e-10) # omega
        self.assertAlmostEqual(YZY[1], 3, delta=1e-10) # chi
        self.assertAlmostEqual(YZY[2], -23, delta=1e-1) # phi

        self.assertAlmostEqual(peaks.getPeak(0).getWavelength(), 1.54, delta=1e-10)


if __name__ == '__main__':
    unittest.main()
