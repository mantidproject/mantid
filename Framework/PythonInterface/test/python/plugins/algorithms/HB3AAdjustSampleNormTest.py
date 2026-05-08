# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import unittest
import numpy as np
from mantid.dataobjects import GroupingWorkspace
from mantid.simpleapi import CreateMDHistoWorkspace, DeleteWorkspace, HB3AAdjustSampleNorm, LoadMD, SliceMDHisto, AddSampleLog, mtd


class HB3AAdjustSampleNormTest(unittest.TestCase):
    _tolerance = 1.0e-7

    def setUp(self):
        return

    def tearDown(self):
        return

    def __checkAdjustments(self, orig_pos, new_pos, height, distance):
        # Check the changed height
        np.testing.assert_allclose(new_pos.getY() - orig_pos.getY(), height, self._tolerance)

        # Check the changed distance along x-z
        dist = np.linalg.norm([new_pos.getX() - orig_pos.getX(), new_pos.getZ() - orig_pos.getZ()])
        np.testing.assert_allclose(dist, distance, self._tolerance)

    def testAdjustDetector(self):
        # Test a slight adjustment of the detector position
        height_adj = 0.75
        dist_adj = 0.25
        orig = LoadMD("HB3A_data.nxs", LoadHistory=False)
        # Get the original detector position before adjustment
        orig_pos = orig.getExperimentInfo(0).getInstrument().getDetector(1).getPos()
        result = HB3AAdjustSampleNorm(InputWorkspaces=orig, DetectorHeightOffset=height_adj, DetectorDistanceOffset=dist_adj)
        # Get the updated detector position
        new_pos = result.getExperimentInfo(0).getInstrument().getDetector(1).getPos()

        # Verify detector adjustment
        self.__checkAdjustments(orig_pos, new_pos, height_adj, dist_adj)

        DeleteWorkspace(orig, result)

    def testDoNotAdjustDetector(self):
        # Ensure detector position does not change when no offsets are given
        orig = LoadMD("HB3A_data.nxs", LoadHistory=False)
        orig_pos = orig.getExperimentInfo(0).getInstrument().getDetector(1).getPos()
        result = HB3AAdjustSampleNorm(InputWorkspaces=orig, DetectorHeightOffset=0.0, DetectorDistanceOffset=0.0)
        new_pos = result.getExperimentInfo(0).getInstrument().getDetector(1).getPos()

        # Verify detector adjustment
        self.__checkAdjustments(orig_pos, new_pos, 0.0, 0.0)

        DeleteWorkspace(orig, result)

    def testInputFail(self):
        signal = range(0, 1000)
        error = range(0, 1000)
        samplews = CreateMDHistoWorkspace(
            Dimensionality=3,
            SignalInput=signal,
            ErrorInput=error,
            Extents="-3,3,-3,3,-3,3",
            NumberOfBins="10,10,10",
            Names="x,y,z",
            Units="MomentumTransfer,EnergyTransfer,EnergyTransfer",
        )

        # A MDHisto WS with no experiment info should fail
        with self.assertRaisesRegex(RuntimeError, "No experiment info was found in 'samplews'"):
            HB3AAdjustSampleNorm(
                InputWorkspaces=samplews, DetectorHeightOffset=0.0, DetectorDistanceOffset=0.0, OutputWorkspace="__tmpout", Wavelength=2.0
            )

        DeleteWorkspace(samplews)

    def testDetectorNormalisation(self):
        data1 = HB3AAdjustSampleNorm("HB3A_data.nxs", OutputType="Detector", NormaliseBy="None")
        self.assertEqual(data1.getSignalArray().max(), 16)
        self.assertEqual(data1.getErrorSquaredArray().max(), 16)

        # normlise by time, data 2 seconds
        data2 = HB3AAdjustSampleNorm("HB3A_data.nxs", OutputType="Detector", NormaliseBy="Time")
        self.assertEqual(data2.getSignalArray().max(), 16 / 2)
        self.assertEqual(data2.getErrorSquaredArray().max(), 16 / 2**2)

        # normalize by monitor, about 621 counts
        data3 = HB3AAdjustSampleNorm("HB3A_data.nxs", OutputType="Detector", NormaliseBy="Monitor")
        self.assertAlmostEqual(data3.getSignalArray().max(), 16 / 621)
        self.assertAlmostEqual(data3.getErrorSquaredArray().max(), 16 / 621**2)

        # create van data
        van = SliceMDHisto(data1, "0,0,0", "1536,512,1")
        van.setSignalArray(np.full_like(van.getSignalArray(), 25))
        van.setErrorSquaredArray(np.full_like(van.getSignalArray(), 25))
        AddSampleLog(van, LogName="time", LogText="42", LogType="Number Series", NumberType="Double")
        AddSampleLog(van, LogName="monitor", LogText="420", LogType="Number Series", NumberType="Double")

        data1 = HB3AAdjustSampleNorm("HB3A_data.nxs", VanadiumWorkspace=van, OutputType="Detector", NormaliseBy="None")
        self.assertAlmostEqual(data1.getSignalArray().max(), 16 / 25)
        self.assertAlmostEqual(data1.getErrorSquaredArray().max(), (16 / 25) ** 2 * (1 / 16 + 1 / 25))

        # normlise by time, data 2 seconds
        data2 = HB3AAdjustSampleNorm("HB3A_data.nxs", VanadiumWorkspace=van, OutputType="Detector", NormaliseBy="Time")
        self.assertAlmostEqual(data2.getSignalArray().max(), 16 / 25 * 42 / 2)
        self.assertAlmostEqual(data2.getErrorSquaredArray().max(), (16 / 25) ** 2 * (1 / 16 + 1 / 25) * (42 / 2) ** 2)

        # normalize by monitor, about 621 counts
        data3 = HB3AAdjustSampleNorm("HB3A_data.nxs", VanadiumWorkspace=van, OutputType="Detector", NormaliseBy="Monitor")
        self.assertAlmostEqual(data3.getSignalArray().max(), 16 / 25 * 420 / 621)
        self.assertAlmostEqual(data3.getErrorSquaredArray().max(), (16 / 25) ** 2 * (1 / 16 + 1 / 25) * (420 / 621) ** 2)

    def testDetectorGrouping(self):
        data = HB3AAdjustSampleNorm("HB3A_data.nxs", OutputType="Detector", NormaliseBy="None", Grouping="None")
        data_2x2 = HB3AAdjustSampleNorm("HB3A_data.nxs", OutputType="Detector", NormaliseBy="None", Grouping="2x2")
        data_4x4 = HB3AAdjustSampleNorm("HB3A_data.nxs", OutputType="Detector", NormaliseBy="None", Grouping="4x4")

        ref_sum = data.getSignalArray().sum()
        self.assertAlmostEqual(ref_sum, data_2x2.getSignalArray().sum())
        self.assertAlmostEqual(ref_sum, data_4x4.getSignalArray().sum())

        ref_shape = data.getSignalArray().shape
        self.assertEqual(ref_shape, tuple([2 * val if i < 2 else val for i, val in enumerate(data_2x2.getSignalArray().shape)]))
        self.assertEqual(ref_shape, tuple([4 * val if i < 2 else val for i, val in enumerate(data_4x4.getSignalArray().shape)]))

        # --- detectorID log checks ---
        run = data.getExperimentInfo(0).run()
        run_2x2 = data_2x2.getExperimentInfo(0).run()
        run_4x4 = data_4x4.getExperimentInfo(0).run()

        # Log must exist for all grouping modes
        for r, label in [(run, "ungrouped"), (run_2x2, "2x2"), (run_4x4, "4x4")]:
            self.assertTrue(r.hasProperty("detectorID"), f"detectorID log missing for {label}")

        ids = np.array(run.getProperty("detectorID").value, dtype=int)
        ids_2x2 = np.array(run_2x2.getProperty("detectorID").value, dtype=int)
        ids_4x4 = np.array(run_4x4.getProperty("detectorID").value, dtype=int)

        # One entry per detector / grouped pixel (y_dim * x_dim)
        self.assertEqual(len(ids), 1536 * 512)
        self.assertEqual(len(ids_2x2), 768 * 256)
        self.assertEqual(len(ids_4x4), 384 * 128)

        # All IDs must be unique within each grouping
        self.assertEqual(len(np.unique(ids)), len(ids), "ungrouped detectorIDs should all be unique")
        self.assertEqual(len(np.unique(ids_2x2)), len(ids_2x2), "2x2 detectorIDs should all be unique")
        self.assertEqual(len(np.unique(ids_4x4)), len(ids_4x4), "4x4 detectorIDs should all be unique")

        # Each grouped ID must be a valid single-pixel ID (group leader is a real detector)
        ungrouped_set = set(ids.tolist())
        self.assertTrue(
            set(ids_2x2.tolist()).issubset(ungrouped_set),
            "Every 2x2 group-leader ID should also appear in the ungrouped detectorID list",
        )
        self.assertTrue(
            set(ids_4x4.tolist()).issubset(ungrouped_set),
            "Every 4x4 group-leader ID should also appear in the ungrouped detectorID list",
        )

        # 4x4 group leaders must also be 2x2 group leaders (coarser grouping is a subset of finer)
        grouped_2x2_set = set(ids_2x2.tolist())
        self.assertTrue(
            set(ids_4x4.tolist()).issubset(grouped_2x2_set),
            "Every 4x4 group-leader ID should also be a 2x2 group-leader ID",
        )

        # Check specific values for the first four entries of each detectorID log.
        # The HB3A instrument (HB3A_Definition_20190926_3.xml) has three 512×512 banks:
        #   Bank 1: idstart=1,      idstepbyrow=512  → detID = 1      + x + y*512
        #   Bank 2: idstart=262145, idstepbyrow=512  → detID = 262145 + x + y*512
        #   Bank 3: idstart=524289, idstepbyrow=512  → detID = 524289 + x + y*512
        # CreateSimulationWorkspace + RewriteSpectraMap=True assigns spectrum k (0-based)
        # to the kth detector in sorted ID order, so spectrum k maps to detector k+1.
        # The reshape(y_dim, x_dim).T.flatten() transform maps:
        #   flat2[c*y_dim + r] = flat_column[r*x_dim + c]
        #
        # Ungrouped (y_dim=1536, x_dim=512): flat_column[k] = k+1
        #   i=0: flat[0]    = 1    (bank1, x=0, y=0)
        #   i=1: flat[512]  = 513  (bank1, x=0, y=1)
        #   i=2: flat[1024] = 1025 (bank1, x=0, y=2)
        #   i=3: flat[1536] = 1537 (bank1, x=0, y=3)
        np.testing.assert_array_equal(ids[:4], [1, 513, 1025, 1537], err_msg="First four ungrouped detectorIDs are incorrect")

        # 2x2 (y_dim=768, x_dim=256): group at (x_idx, y_idx) has first spectrum 2*x_idx + 1024*y_idx
        #   → detID = 2*x_idx + 1024*y_idx + 1
        #   flat2 index i → k = r*256 + c (r = i % 768, c = i // 768):
        #   i=0: k=0,   x_idx=0, y_idx=0   → 1      (bank1, x=0, y=0)
        #   i=1: k=256, x_idx=0, y_idx=256  → 262145 (bank2, x=0, y=0 — first pixel of bank 2)
        #   i=2: k=512, x_idx=0, y_idx=512  → 524289 (bank3, x=0, y=0 — first pixel of bank 3)
        #   i=3: k=768, x_idx=1, y_idx=0    → 3      (bank1, x=2, y=0)
        np.testing.assert_array_equal(ids_2x2[:4], [1, 262145, 524289, 3], err_msg="First four 2x2 detectorIDs are incorrect")

        # 4x4 (y_dim=384, x_dim=128): group at (x_idx, y_idx) has first spectrum 4*x_idx + 2048*y_idx
        #   → detID = 4*x_idx + 2048*y_idx + 1
        #   i=0: k=0,   x_idx=0, y_idx=0   → 1      (bank1, x=0, y=0)
        #   i=1: k=128, x_idx=0, y_idx=128  → 262145 (bank2, x=0, y=0)
        #   i=2: k=256, x_idx=0, y_idx=256  → 524289 (bank3, x=0, y=0)
        #   i=3: k=384, x_idx=1, y_idx=0    → 5      (bank1, x=4, y=0)
        np.testing.assert_array_equal(ids_4x4[:4], [1, 262145, 524289, 5], err_msg="First four 4x4 detectorIDs are incorrect")

    def testOutputGroupingWorkspace(self):

        # --- Validation: OutputGroupingWorkspace requires Grouping != 'None' ---
        with self.assertRaisesRegex(RuntimeError, "OutputGroupingWorkspace"):
            HB3AAdjustSampleNorm(
                "HB3A_data.nxs",
                OutputType="Detector",
                NormaliseBy="None",
                Grouping="None",
                OutputWorkspace="__hb3a_out_fail",
                OutputGroupingWorkspace="__hb3a_grp_fail",
            )

        # --- 2x2: workspace is produced with correct type and size ---
        HB3AAdjustSampleNorm(
            "HB3A_data.nxs",
            OutputType="Detector",
            NormaliseBy="None",
            Grouping="2x2",
            OutputWorkspace="__hb3a_out_2x2",
            OutputGroupingWorkspace="__hb3a_grp_2x2",
        )
        self.assertTrue(mtd.doesExist("__hb3a_grp_2x2"), "OutputGroupingWorkspace should be stored in the ADS")
        grp_2x2 = mtd["__hb3a_grp_2x2"]
        self.assertIsInstance(grp_2x2, GroupingWorkspace)
        # One spectrum per ungrouped detector: 512 columns × 1536 rows (3 × 512 banks)
        self.assertEqual(grp_2x2.getNumberHistograms(), 512 * 1536)

        # Group 1: x in [0,1], y in [0,1]
        #   x_idx=0, y_idx=0, y_groups = 1536//2 = 768  →  group_id = 0*768 + 0 + 1 = 1
        #   Spectrum indices: x+i + (y+j)*512, i,j ∈ {0,1}  →  0, 1, 512, 513
        for i in range(2):
            for j in range(2):
                spec = i + j * 512
                self.assertAlmostEqual(grp_2x2.readY(spec)[0], 1.0, msg=f"Spectrum {spec} should belong to group 1 (2x2)")

        # Group 2: x in [0,1], y in [2,3]
        #   x_idx=0, y_idx=1  →  group_id = 0*768 + 1 + 1 = 2
        #   Spectrum indices: i + (2+j)*512, i,j ∈ {0,1}  →  1024, 1025, 1536, 1537
        for i in range(2):
            for j in range(2):
                spec = i + (2 + j) * 512
                self.assertAlmostEqual(grp_2x2.readY(spec)[0], 2.0, msg=f"Spectrum {spec} should belong to group 2 (2x2)")

        # --- 4x4: workspace is produced with correct type and size ---
        HB3AAdjustSampleNorm(
            "HB3A_data.nxs",
            OutputType="Detector",
            NormaliseBy="None",
            Grouping="4x4",
            OutputWorkspace="__hb3a_out_4x4",
            OutputGroupingWorkspace="__hb3a_grp_4x4",
        )
        self.assertTrue(mtd.doesExist("__hb3a_grp_4x4"), "OutputGroupingWorkspace should be stored in the ADS")
        grp_4x4 = mtd["__hb3a_grp_4x4"]
        self.assertIsInstance(grp_4x4, GroupingWorkspace)
        self.assertEqual(grp_4x4.getNumberHistograms(), 512 * 1536)

        # Group 1: x in [0,3], y in [0,3]
        #   x_idx=0, y_idx=0, y_groups = 1536//4 = 384  →  group_id = 0*384 + 0 + 1 = 1
        #   Spectrum indices: i + j*512, i,j ∈ {0,1,2,3}
        for i in range(4):
            for j in range(4):
                spec = i + j * 512
                self.assertAlmostEqual(grp_4x4.readY(spec)[0], 1.0, msg=f"Spectrum {spec} should belong to group 1 (4x4)")

        # Group 2: x in [0,3], y in [4,7]
        #   x_idx=0, y_idx=1  →  group_id = 0*384 + 1 + 1 = 2
        #   Spectrum indices: i + (4+j)*512, i,j ∈ {0,1,2,3}
        for i in range(4):
            for j in range(4):
                spec = i + (4 + j) * 512
                self.assertAlmostEqual(grp_4x4.readY(spec)[0], 2.0, msg=f"Spectrum {spec} should belong to group 2 (4x4)")

        DeleteWorkspace("__hb3a_out_2x2", "__hb3a_grp_2x2", "__hb3a_out_4x4", "__hb3a_grp_4x4")


if __name__ == "__main__":
    unittest.main()
