# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import unittest
import numpy as np
from mantid.dataobjects import GroupingWorkspace
from mantid.simpleapi import (
    CreateMDHistoWorkspace,
    DeleteWorkspace,
    DeleteWorkspaces,
    HB3AAdjustSampleNorm,
    LoadMD,
    SliceMDHisto,
    AddSampleLog,
    mtd,
)


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

        DeleteWorkspaces([orig, result])

    def testDoNotAdjustDetector(self):
        # Ensure detector position does not change when no offsets are given
        orig = LoadMD("HB3A_data.nxs", LoadHistory=False)
        orig_pos = orig.getExperimentInfo(0).getInstrument().getDetector(1).getPos()
        result = HB3AAdjustSampleNorm(InputWorkspaces=orig, DetectorHeightOffset=0.0, DetectorDistanceOffset=0.0)
        new_pos = result.getExperimentInfo(0).getInstrument().getDetector(1).getPos()

        # Verify detector adjustment
        self.__checkAdjustments(orig_pos, new_pos, 0.0, 0.0)

        DeleteWorkspaces([orig, result])

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
        data_2x2 = HB3AAdjustSampleNorm(
            "HB3A_data.nxs", OutputWorkspace="data_2x2", OutputType="Detector", NormaliseBy="None", Grouping="2x2"
        )
        data_4x4 = HB3AAdjustSampleNorm(
            "HB3A_data.nxs", OutputWorkspace="data_4x4", OutputType="Detector", NormaliseBy="None", Grouping="4x4"
        )

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
        np.testing.assert_array_equal(ids[:4], [1, 513, 1025, 1537])
        np.testing.assert_array_equal(ids_2x2[:4], [1, 1025, 2049, 3073])
        np.testing.assert_array_equal(ids_4x4[:4], [1, 2049, 4097, 6145])

        # clean up
        DeleteWorkspaces([data, data_2x2, data_4x4])

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

        # --- 2x2:
        HB3AAdjustSampleNorm(
            "HB3A_data.nxs",
            OutputType="Detector",
            NormaliseBy="None",
            Grouping="2x2",
            OutputWorkspace="__hb3a_out_2x2",
            OutputGroupingWorkspace="__hb3a_grp_2x2",
        )
        grp_2x2 = mtd["__hb3a_grp_2x2"]
        self.assertIsInstance(grp_2x2, GroupingWorkspace)
        # One spectrum per pixel detector. HB3A has three 512x512 detector panels
        self.assertEqual(grp_2x2.getNumberHistograms(), 512 * 512 * 3)
        # check the group ID of the first spectra:
        group_ids = grp_2x2.extractY().astype(int).flatten()  # 1, 1, 2, 2, 3, 3,..
        self.assertListEqual(group_ids[0:6].tolist(), [1, 1, 2, 2, 3, 3])  # first row along X
        self.assertListEqual(group_ids[512:518].tolist(), [1, 1, 2, 2, 3, 3])  # second row
        # detector ID stored
        histogram_count = grp_2x2.getNumberHistograms()
        self.assertEqual(grp_2x2.getSpectrum(0).getDetectorIDs()[0], 1)
        self.assertEqual(grp_2x2.getSpectrum(histogram_count - 1).getDetectorIDs()[0], histogram_count)

        # --- 4x4:
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
        self.assertEqual(grp_4x4.getNumberHistograms(), 512 * 512 * 3)
        group_ids = grp_4x4.extractY().astype(int).flatten()  # 1, 1, 1, 1, 2, 2, 2, 2,..
        self.assertListEqual(group_ids[0:8].tolist(), [1, 1, 1, 1, 2, 2, 2, 2])  # first row along X
        self.assertListEqual(group_ids[512:520].tolist(), [1, 1, 1, 1, 2, 2, 2, 2])  # second row
        self.assertListEqual(group_ids[1024:1032].tolist(), [1, 1, 1, 1, 2, 2, 2, 2])  # third row
        self.assertListEqual(group_ids[1536:1544].tolist(), [1, 1, 1, 1, 2, 2, 2, 2])  # fourth row
        # detector ID stored
        histogram_count = grp_4x4.getNumberHistograms()
        self.assertEqual(grp_4x4.getSpectrum(0).getDetectorIDs()[0], 1)
        self.assertEqual(grp_4x4.getSpectrum(histogram_count - 1).getDetectorIDs()[0], histogram_count)

        # clean up
        DeleteWorkspaces(["__hb3a_out_2x2", "__hb3a_grp_2x2", "__hb3a_out_4x4", "__hb3a_grp_4x4"])


if __name__ == "__main__":
    unittest.main()
