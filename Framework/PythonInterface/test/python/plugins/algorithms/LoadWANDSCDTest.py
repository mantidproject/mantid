# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from mantid.dataobjects import GroupingWorkspace
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

        # check the sample environment logs
        SE_logs = [log for log in run.keys() if log.startswith("HB2C:SE")]
        self.assertEqual(len(SE_logs), 17)

        # for this test data all log values are 0 except for "HB2C:SE:LS:SETP1" which is 300
        for log in SE_logs:
            prop = run.getProperty(log)
            if log == "HB2C:SE:LS:SETP1":
                np.testing.assert_array_equal(prop.value, [300, 300])
            else:
                np.testing.assert_array_equal(prop.value, [0, 0])

            # check the units of the sample environment logs
            if "MAGB" in log:
                self.assertEqual(prop.units, "T")
            elif "ND1" in log or "Select" in log:
                self.assertEqual(prop.units, "")
            else:
                self.assertEqual(prop.units, "K")

        # test that the goniometer has been set correctly
        self.assertEqual(run.getNumGoniometers(), 2)
        self.assertAlmostEqual(run.getGoniometer(0).getEulerAngles("YXZ")[0], -142.6)  # s1 from HB2C_7000
        self.assertAlmostEqual(run.getGoniometer(1).getEulerAngles("YXZ")[0], -142.5)  # s1 from HB2C_7001
        self.assertAlmostEqual(run.getGoniometer(0).getEulerAngles("YXZ")[2], -4.4)  # sgu from HB2C_7000
        self.assertAlmostEqual(run.getGoniometer(1).getEulerAngles("YXZ")[2], -4.4)  # sgu from HB2C_7001
        self.assertAlmostEqual(run.getGoniometer(0).getEulerAngles("YXZ")[1], 4.6995)  # sgl from HB2C_7000
        self.assertAlmostEqual(run.getGoniometer(1).getEulerAngles("YXZ")[1], 4.6995)  # sgl from HB2C_7001

        # check detectorID log: length must match twotheta/azimuthal, values must be non-negative
        twotheta = run.getProperty("twotheta").value
        azimuthal = run.getProperty("azimuthal").value
        detectorID = run.getProperty("detectorID").value
        self.assertEqual(len(detectorID), len(twotheta))
        self.assertEqual(len(detectorID), len(azimuthal))
        self.assertTrue(all(d >= 0 for d in detectorID))

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
    # Full detector grid dimensions for WAND (HB2C)
    N_ROWS = 480 * 8  # 3840
    N_COLS = 512
    TOTAL_COUNTS = 128049  # number of events in file "HB2C_475936.nxs.h5"

    def test_grouping_workspace_2x2(self):
        grouping = 2
        ws, grp_ws = LoadWANDSCD("HB2C_475936.nxs.h5", Grouping="2x2", OutputWorkspace="ws_2x2", OutputGroupingWorkspace="grp_ws_2x2")

        self.assertEqual(ws.getSignalArray().sum(), self.TOTAL_COUNTS)

        # Type check
        self.assertIsInstance(grp_ws, GroupingWorkspace)

        # Total number of groups must equal (N_ROWS // g) * (N_COLS // g)
        expected_groups = (self.N_ROWS // grouping) * (self.N_COLS // grouping)
        y_values = grp_ws.extractY().flatten()
        self.assertEqual(int(y_values.max()), expected_groups)

        # Every workspace index lies in a valid group (1 .. expected_groups)
        histogram_count = grp_ws.getNumberHistograms()
        self.assertEqual(histogram_count, self.N_ROWS * self.N_COLS)
        self.assertEqual(int(y_values.min()), 1)

        # Spot-check: workspace index 0  → pixel (x=0, y=0) → group 1
        self.assertEqual(int(grp_ws.readY(0)[0]), 1)
        # Spot-check: workspace index 1  → pixel (x=0, y=1) → group 1  (same 2×2 block as index 0)
        self.assertEqual(int(grp_ws.readY(1)[0]), 1)
        # Spot-check: workspace index 2  → pixel (x=0, y=2) → group 2  (next 2×2 block along y)
        self.assertEqual(int(grp_ws.readY(2)[0]), 2)
        # Spot-check: workspace index 512 → pixel (x=1, y=0) → shares 2×2 block with index 0 → group 1
        self.assertEqual(int(grp_ws.readY(512)[0]), 1)
        # Spot-check: workspace index 1024 → pixel (x=2, y=0) → first group of x_idx=1 row → group (512//2)+1 = 257
        self.assertEqual(int(grp_ws.readY(1024)[0]), (self.N_COLS // grouping) + 1)

        # detector ID stored
        self.assertEqual(grp_ws.getSpectrum(0).getDetectorIDs()[0], 0)
        self.assertEqual(grp_ws.getSpectrum(histogram_count - 1).getDetectorIDs()[0], histogram_count - 1)

        # detectorID log must have one entry per group (same length as twotheta/azimuthal)
        run = ws.getExperimentInfo(0).run()
        detectorID = run.getProperty("detectorID").value
        twotheta = run.getProperty("twotheta").value
        self.assertEqual(len(detectorID), expected_groups)
        self.assertEqual(len(detectorID), len(twotheta))
        # each stored detector ID maps to a valid group (det_id == workspace index for WAND)
        self.assertTrue(all(grp_ws.readY(int(d))[0] >= 1 for d in detectorID))

        ws.delete()
        grp_ws.delete()

    def test_grouping_workspace_4x4(self):
        grouping = 4
        ws, grp_ws = LoadWANDSCD("HB2C_475936.nxs.h5", Grouping="4x4", OutputGroupingWorkspace="grp_ws_4x4")

        self.assertEqual(ws.getSignalArray().sum(), self.TOTAL_COUNTS)

        self.assertIsInstance(grp_ws, GroupingWorkspace)
        expected_groups = (self.N_ROWS // grouping) * (self.N_COLS // grouping)
        y_values = grp_ws.extractY().flatten()
        self.assertEqual(int(y_values.max()), expected_groups)

        # Group size: each group spans g×g = 16 detectors
        group_size = grouping * grouping
        indices_in_first_group = np.where(y_values == 1.0)[0]
        self.assertEqual(len(indices_in_first_group), group_size)

        ws.delete()
        grp_ws.delete()

    def test_validation_rejects_grouping_ws_without_grouping(self):
        """Requesting OutputGroupingWorkspace while Grouping='None' must raise."""
        with self.assertRaises(Exception):
            LoadWANDSCD("HB2C_475936.nxs.h5", Grouping="None", OutputGroupingWorkspace="should_fail")


if __name__ == "__main__":
    unittest.main()
