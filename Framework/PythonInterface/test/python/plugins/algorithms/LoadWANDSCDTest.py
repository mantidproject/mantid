# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from mantid.simpleapi import LoadWANDSCD
import unittest
import numpy as np


class LoadWANDTest(unittest.TestCase):
    def test(self):
        LoadWANDTest_ws = LoadWANDSCD("HB2C_7000.nxs.h5,HB2C_7001.nxs.h5")
        self.assertTrue(LoadWANDTest_ws)
        self.assertEqual(LoadWANDTest_ws.getNumDims(), 3)
        self.assertEqual(LoadWANDTest_ws.getNPoints(), 1966080 * 2)
        self.assertEqual(LoadWANDTest_ws.getSignalArray().max(), 7)

        d0 = LoadWANDTest_ws.getDimension(0)
        self.assertEqual(d0.name, "y")
        self.assertEqual(d0.getNBins(), 512)
        self.assertEqual(d0.getMinimum(), 0.5)
        self.assertEqual(d0.getMaximum(), 512.5)

        d1 = LoadWANDTest_ws.getDimension(1)
        self.assertEqual(d1.name, "x")
        self.assertEqual(d1.getNBins(), 3840)
        self.assertEqual(d1.getMinimum(), 0.5)
        self.assertEqual(d1.getMaximum(), 3840.5)

        d2 = LoadWANDTest_ws.getDimension(2)
        self.assertEqual(d2.name, "scanIndex")
        self.assertEqual(d2.getNBins(), 2)
        self.assertEqual(d2.getMinimum(), 0.5)
        self.assertEqual(d2.getMaximum(), 2.5)

        self.assertEqual(LoadWANDTest_ws.getNumExperimentInfo(), 1)
        self.assertEqual(LoadWANDTest_ws.getExperimentInfo(0).getInstrument().getName(), "WAND")

        run = LoadWANDTest_ws.getExperimentInfo(0).run()
        s1 = run.getProperty("HB2C:Mot:s1").value
        self.assertEqual(len(s1), 2)
        self.assertAlmostEqual(s1[0], -142.6)
        self.assertAlmostEqual(s1[1], -142.5)
        sgl = run.getProperty("HB2C:Mot:sgl").value
        self.assertEqual(len(sgl), 2)
        self.assertAlmostEqual(sgl[0], -4.6995)
        self.assertAlmostEqual(sgl[1], -4.6995)
        sgu = run.getProperty("HB2C:Mot:sgu").value
        self.assertEqual(len(sgu), 2)
        self.assertAlmostEqual(sgu[0], 4.4)
        self.assertAlmostEqual(sgu[1], 4.4)
        run_number = run.getProperty("run_number").value
        self.assertEqual(len(run_number), 2)
        self.assertEqual(run_number[0], 7000)
        self.assertEqual(run_number[1], 7001)
        monitor_count = run.getProperty("monitor_count").value
        self.assertEqual(len(monitor_count), 2)
        self.assertEqual(monitor_count[0], 907880)
        self.assertEqual(monitor_count[1], 908651)
        duration = run.getProperty("duration").value
        self.assertEqual(len(duration), 2)
        self.assertAlmostEqual(duration[0], 40.05, 5)
        self.assertAlmostEqual(duration[1], 40.05, 5)

        # test that the goniometer has been set correctly
        self.assertEqual(run.getNumGoniometers(), 2)
        self.assertAlmostEqual(run.getGoniometer(0).getEulerAngles("YXZ")[0], -142.6)  # s1 from HB2C_7000
        self.assertAlmostEqual(run.getGoniometer(1).getEulerAngles("YXZ")[0], -142.5)  # s1 from HB2C_7001
        self.assertAlmostEqual(run.getGoniometer(0).getEulerAngles("YXZ")[2], -4.4)  # sgu from HB2C_7000
        self.assertAlmostEqual(run.getGoniometer(1).getEulerAngles("YXZ")[2], -4.4)  # sgu from HB2C_7001
        self.assertAlmostEqual(run.getGoniometer(0).getEulerAngles("YXZ")[1], 4.6995)  # sgl from HB2C_7000
        self.assertAlmostEqual(run.getGoniometer(1).getEulerAngles("YXZ")[1], 4.6995)  # sgl from HB2C_7001
        LoadWANDTest_ws.delete()


class LoadWANDApplyGoniometerTiltTest(unittest.TestCase):
    def test(self):
        sgl, sgu = np.deg2rad(-6.2), np.deg2rad(-0.4)  # radian
        UB = np.array([[0.0167, -0.0181, -0.0762], [-0.2218, -0.1959, -0.0009], [-0.0968, 0.1419, -0.011]])  # crystal align
        Rx = np.array([[1, 0, 0], [0, np.cos(sgl), np.sin(sgl)], [0, -np.sin(sgl), np.cos(sgl)]])  # 'HB2C:Mot:sgl.RBV,1,0,0,-1'
        Rz = np.array([[np.cos(sgu), np.sin(sgu), 0], [-np.sin(sgu), np.cos(sgu), 0], [0, 0, 1]])  # 'HB2C:Mot:sgu.RBV,0,0,1,-1'

        LoadWANDTest_ws = LoadWANDSCD("HB2C_475936.nxs.h5", ApplyGoniometerTilt=False)
        np.testing.assert_array_almost_equal(UB, LoadWANDTest_ws.getExperimentInfo(0).sample().getOrientedLattice().getUB())
        LoadWANDTest_ws.delete()

        LoadWANDTest_ws = LoadWANDSCD("HB2C_475936.nxs.h5", ApplyGoniometerTilt=True)
        np.testing.assert_array_almost_equal(Rx @ Rz @ UB, LoadWANDTest_ws.getExperimentInfo(0).sample().getOrientedLattice().getUB())
        LoadWANDTest_ws.delete()


class LoadWANDGrouping(unittest.TestCase):
    def test(self):
        LoadWANDTest_ws = LoadWANDSCD("HB2C_475936.nxs.h5", Grouping=None)
        LoadWANDTest_ws_2x2 = LoadWANDSCD("HB2C_475936.nxs.h5", Grouping="2x2")
        LoadWANDTest_ws_4x4 = LoadWANDSCD("HB2C_475936.nxs.h5", Grouping="4x4")

        self.assertEqual(LoadWANDTest_ws.getSignalArray().sum(), LoadWANDTest_ws_2x2.getSignalArray().sum())
        self.assertEqual(LoadWANDTest_ws.getSignalArray().sum(), LoadWANDTest_ws_4x4.getSignalArray().sum())

        LoadWANDTest_ws.delete()
        LoadWANDTest_ws_2x2.delete()
        LoadWANDTest_ws_4x4.delete()


if __name__ == "__main__":
    unittest.main()
