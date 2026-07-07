# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import unittest
import numpy as np
from mantid.api import AlgorithmManager
from mantid.dataobjects import GroupingWorkspace
from mantid.simpleapi import (
    CreateMDHistoWorkspace,
    DeleteWorkspace,
    DeleteWorkspaces,
    HB3AAdjustSampleNorm,
    LoadMD,
    QueryMDWorkspace,
    SliceMDHisto,
    AddSampleLog,
    mtd,
)


class HB3AAdjustSampleNormTest(unittest.TestCase):
    _tolerance = 1.0e-7
    _input_ws = "__hb3a_shared_input"
    _van_ws = "__hb3a_shared_van"

    @classmethod
    def setUpClass(cls):
        LoadMD("HB3A_data.nxs", LoadHistory=False, OutputWorkspace=cls._input_ws)
        norm_source = HB3AAdjustSampleNorm(
            InputWorkspaces=cls._input_ws, OutputWorkspace="__hb3a_shared_norm_source", OutputType="Detector", NormaliseBy="None"
        )
        cls.__createVanadiumWorkspace(norm_source, cls._van_ws)
        DeleteWorkspace(norm_source)

    @classmethod
    def tearDownClass(cls):
        for workspace in (cls._input_ws, cls._van_ws):
            if mtd.doesExist(workspace):
                DeleteWorkspace(workspace)

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

    @staticmethod
    def __createVanadiumWorkspace(source_ws, output_ws):
        van = SliceMDHisto(source_ws, "0,0,0", "1536,512,1", OutputWorkspace=output_ws)
        van.setSignalArray(np.full_like(van.getSignalArray(), 25))
        van.setErrorSquaredArray(np.full_like(van.getSignalArray(), 25))
        AddSampleLog(van, LogName="time", LogText="42", LogType="Number Series", NumberType="Double")
        AddSampleLog(van, LogName="monitor", LogText="420", LogType="Number Series", NumberType="Double")
        return van

    def testAdjustDetector(self):
        # Test a slight adjustment of the detector position
        height_adj = 0.75
        dist_adj = 0.25
        # Get the original detector position before adjustment
        orig_pos = mtd[self._input_ws].getExperimentInfo(0).getInstrument().getDetector(1).getPos()
        result = HB3AAdjustSampleNorm(InputWorkspaces=self._input_ws, DetectorHeightOffset=height_adj, DetectorDistanceOffset=dist_adj)
        # Get the updated detector position
        new_pos = result.getExperimentInfo(0).getInstrument().getDetector(1).getPos()

        # Verify detector adjustment
        self.__checkAdjustments(orig_pos, new_pos, height_adj, dist_adj)

        DeleteWorkspace(result)

    def testDoNotAdjustDetector(self):
        # Ensure detector position does not change when no offsets are given
        orig_pos = mtd[self._input_ws].getExperimentInfo(0).getInstrument().getDetector(1).getPos()
        result = HB3AAdjustSampleNorm(InputWorkspaces=self._input_ws, DetectorHeightOffset=0.0, DetectorDistanceOffset=0.0)
        new_pos = result.getExperimentInfo(0).getInstrument().getDetector(1).getPos()

        # Verify detector adjustment
        self.__checkAdjustments(orig_pos, new_pos, 0.0, 0.0)

        DeleteWorkspace(result)

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

    def testDetectorNormalizeDataFalse(self):
        data = HB3AAdjustSampleNorm(
            InputWorkspaces=self._input_ws,
            VanadiumWorkspace=mtd[self._van_ws],
            OutputWorkspace="__hb3a_detector_no_norm",
            OutputType="Detector",
            NormaliseBy="Monitor",
            NormalizeData=False,
        )

        self.assertEqual(data.getSignalArray().max(), 16)
        self.assertEqual(data.getErrorSquaredArray().max(), 16)

        DeleteWorkspace(data)

    def testQSampleHistogramNormalizeDataFalse(self):
        data = HB3AAdjustSampleNorm(
            InputWorkspaces=self._input_ws,
            VanadiumWorkspace=mtd[self._van_ws],
            OutputWorkspace="__hb3a_histo_no_norm",
            OutputType="Q-sample histogram",
            NormaliseBy="Monitor",
            NormalizeData=False,
            BinningDim0="-5.0125,5.0125,101",
            BinningDim1="-2.0125,3.0125,51",
            BinningDim2="-0.0125,5.0125,51",
        )

        self.assertEqual(data.getNumDims(), 3)
        self.assertGreater(np.nanmax(data.getSignalArray()), 0)

        DeleteWorkspace(data)

    def testQSampleEventsNormalizeDataFalse(self):
        data = HB3AAdjustSampleNorm(
            InputWorkspaces=self._input_ws,
            VanadiumWorkspace=mtd[self._van_ws],
            OutputWorkspace="__hb3a_events_no_norm",
            OutputType="Q-sample events",
            NormaliseBy="Monitor",
            NormalizeData=False,
            ScaleByMotorStep=True,
        )

        self.assertEqual(data.getNumDims(), 3)
        self.assertEqual(data.getNEvents(), 9038)

        DeleteWorkspace(data)

    def testValidateInputs(self):
        # User passes an OutputNormalizationWorkspace but the conditions are not met
        alg = AlgorithmManager.createUnmanaged("HB3AAdjustSampleNorm")
        alg.initialize()
        alg.setProperty("Filename", "HB3A_data.nxs")
        alg.setProperty("OutputWorkspace", "__hb3a_out")
        alg.setProperty("OutputNormalizationWorkspace", "__hb3a_norm")
        issues = alg.validateInputs()
        self.assertIn("OutputNormalizationWorkspace", issues)

        # User passes MergeInputs=True with a single input file
        alg2 = AlgorithmManager.createUnmanaged("HB3AAdjustSampleNorm")
        alg2.initialize()
        alg2.setProperty("Filename", "HB3A_data.nxs")
        alg2.setProperty("OutputWorkspace", "__hb3a_out")
        alg2.setProperty("MergeInputs", True)
        issues2 = alg2.validateInputs()
        self.assertIn("MergeInputs", issues2)

    def testDetectorNormalisation(self):
        data1 = HB3AAdjustSampleNorm(
            InputWorkspaces=self._input_ws, OutputWorkspace="__hb3a_detector_none", OutputType="Detector", NormaliseBy="None"
        )
        self.assertEqual(data1.getSignalArray().max(), 16)
        self.assertEqual(data1.getErrorSquaredArray().max(), 16)

        # normlise by time, data 2 seconds
        data2 = HB3AAdjustSampleNorm(
            InputWorkspaces=self._input_ws, OutputWorkspace="__hb3a_detector_time", OutputType="Detector", NormaliseBy="Time"
        )
        self.assertEqual(data2.getSignalArray().max(), 16 / 2)
        self.assertEqual(data2.getErrorSquaredArray().max(), 16 / 2**2)

        # normalize by monitor, about 621 counts
        data3 = HB3AAdjustSampleNorm(
            InputWorkspaces=self._input_ws, OutputWorkspace="__hb3a_detector_monitor", OutputType="Detector", NormaliseBy="Monitor"
        )
        self.assertAlmostEqual(data3.getSignalArray().max(), 16 / 621)
        self.assertAlmostEqual(data3.getErrorSquaredArray().max(), 16 / 621**2)

        data4 = HB3AAdjustSampleNorm(
            InputWorkspaces=self._input_ws,
            VanadiumWorkspace=mtd[self._van_ws],
            OutputWorkspace="__hb3a_detector_van_none",
            OutputType="Detector",
            NormaliseBy="None",
        )
        self.assertAlmostEqual(data4.getSignalArray().max(), 16 / 25)
        self.assertAlmostEqual(data4.getErrorSquaredArray().max(), (16 / 25) ** 2 * (1 / 16 + 1 / 25))

        # normlise by time, data 2 seconds
        data5 = HB3AAdjustSampleNorm(
            InputWorkspaces=self._input_ws,
            VanadiumWorkspace=mtd[self._van_ws],
            OutputWorkspace="__hb3a_detector_van_time",
            OutputType="Detector",
            NormaliseBy="Time",
        )
        self.assertAlmostEqual(data5.getSignalArray().max(), 16 / 25 * 42 / 2)
        self.assertAlmostEqual(data5.getErrorSquaredArray().max(), (16 / 25) ** 2 * (1 / 16 + 1 / 25) * (42 / 2) ** 2)

        # normalize by monitor, about 621 counts
        data6 = HB3AAdjustSampleNorm(
            InputWorkspaces=self._input_ws,
            VanadiumWorkspace=mtd[self._van_ws],
            OutputWorkspace="__hb3a_detector_van_monitor",
            OutputType="Detector",
            NormaliseBy="Monitor",
        )
        self.assertAlmostEqual(data6.getSignalArray().max(), 16 / 25 * 420 / 621)
        self.assertAlmostEqual(data6.getErrorSquaredArray().max(), (16 / 25) ** 2 * (1 / 16 + 1 / 25) * (420 / 621) ** 2)

        DeleteWorkspaces([data1, data2, data3, data4, data5, data6])

    def testDetectorGrouping(self):
        data = HB3AAdjustSampleNorm(
            InputWorkspaces=self._input_ws, OutputWorkspace="data", OutputType="Detector", NormaliseBy="None", Grouping="None"
        )
        data_2x2 = HB3AAdjustSampleNorm(
            InputWorkspaces=self._input_ws, OutputWorkspace="data_2x2", OutputType="Detector", NormaliseBy="None", Grouping="2x2"
        )
        data_4x4 = HB3AAdjustSampleNorm(
            InputWorkspaces=self._input_ws, OutputWorkspace="data_4x4", OutputType="Detector", NormaliseBy="None", Grouping="4x4"
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
                InputWorkspaces=self._input_ws,
                OutputType="Detector",
                NormaliseBy="None",
                Grouping="None",
                OutputWorkspace="__hb3a_out_fail",
                OutputGroupingWorkspace="__hb3a_grp_fail",
            )

        # --- 2x2:
        HB3AAdjustSampleNorm(
            InputWorkspaces=self._input_ws,
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
            InputWorkspaces=self._input_ws,
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

    def testOutputNormalizationWorkspace(self):
        """Verify that OutputNormalizationWorkspace is produced when NormalizeData is False,
        OutputType is 'Q-sample events', and vanadium data is provided.
        Both workspaces must be 3-D MDEvent workspaces. The normalization workspace is built
        from a uniformly non-zero vanadium replicated across all detector pixels, so it has at
        least as many events as the data workspace (which only contains pixels with non-zero
        counts). The signal per event in each non-empty box of the normalization workspace
        must equal vanws_signal * data_monitor / van_monitor, which is the per-step vanadium
        intensity scaled by the monitor flux ratio used during normalization."""
        HB3AAdjustSampleNorm(
            InputWorkspaces=self._input_ws,
            VanadiumWorkspace=mtd[self._van_ws],
            OutputWorkspace="__hb3a_events_norm_ws",
            OutputNormalizationWorkspace="__hb3a_norm_ws",
            OutputType="Q-sample events",
            NormaliseBy="Monitor",
            NormalizeData=False,
        )
        data = mtd["__hb3a_events_norm_ws"]
        norm = mtd["__hb3a_norm_ws"]

        self.assertEqual(data.getNumDims(), 3)
        self.assertEqual(norm.getNumDims(), 3)
        self.assertGreater(norm.getNEvents(), 0)
        self.assertLessEqual(data.getNEvents(), norm.getNEvents())

        norm_table = QueryMDWorkspace(InputWorkspace=norm, Normalisation="none", LimitRows=False)
        events_per_box = np.asarray(norm_table.column("Number of Events"), dtype=float)
        signal_per_box = np.asarray(norm_table.column("Signal/none"), dtype=float)
        non_empty_boxes = events_per_box > 0
        monitor_values = data.getExperimentInfo(0).run().getProperty("monitor").value
        vanadium_monitor = mtd[self._van_ws].getExperimentInfo(0).run().getProperty("monitor").value[0]
        expected_signal_per_event = 25 * monitor_values.mean() / vanadium_monitor
        # Each box may contain events from a single scan step, each with its own monitor count.
        # rtol=0.04 covers the maximum relative deviation of any per-step monitor from the mean
        # (monitor counts span 614-659 across 13 steps, a ~4% spread around the mean of 635).
        np.testing.assert_allclose(
            signal_per_box[non_empty_boxes] / events_per_box[non_empty_boxes],
            expected_signal_per_event,
            rtol=0.04,
        )

        DeleteWorkspaces(["__hb3a_events_norm_ws", "__hb3a_norm_ws", norm_table])


if __name__ == "__main__":
    unittest.main()
