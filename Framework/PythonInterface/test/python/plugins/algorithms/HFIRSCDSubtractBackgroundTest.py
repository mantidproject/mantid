# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2026 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import unittest

import numpy as np

from mantid.api import WorkspaceFactory, WorkspaceGroup, mtd
from mantid.simpleapi import (
    CloneWorkspace,
    CreateMDHistoWorkspace,
    CreateSampleWorkspace,
    DeleteWorkspace,
    GroupWorkspaces,
    HFIRSCDSubtractBackground,
    LoadEmptyInstrument,
)


class HFIRSCDSubtractBackgroundTest(unittest.TestCase):
    # sample: 2 x 3 pixels and 4 goniometer angles; background: same pixels and one goniometer angle
    SAMPLE_SIGNAL = np.arange(1.0, 25.0).reshape((2, 3, 4)) * 10.0
    SAMPLE_ERROR_SQUARED = np.arange(1.0, 25.0).reshape((2, 3, 4))
    BACKGROUND_SIGNAL = np.array([[5.0, 10.0, 15.0], [20.0, 25.0, 30.0]]).reshape((2, 3, 1))
    BACKGROUND_ERROR_SQUARED = np.array([[0.5, 1.0, 1.5], [2.0, 2.5, 3.0]]).reshape((2, 3, 1))
    SAMPLE_FLUX = [100.0, 200.0, 300.0, 400.0]
    BACKGROUND_FLUX = [50.0]
    FLUX_RATIO = np.array([2.0, 4.0, 6.0, 8.0])  # SAMPLE_FLUX / BACKGROUND_FLUX
    FLUX_LOGS = {"WAND": {"Time": "duration", "Monitor": "monitor_count"}, "DEMAND": {"Time": "time", "Monitor": "monitor"}}

    @classmethod
    def setUpClass(cls):
        # Load each instrument once; every workspace gets its own clone, so that sample logs are not shared
        cls.instruments = {
            "WAND": LoadEmptyInstrument(InstrumentName="HB2C", OutputWorkspace="__wand_instrument"),
            "DEMAND": LoadEmptyInstrument(InstrumentName="HB3A", OutputWorkspace="__demand_instrument"),
        }

    @classmethod
    def tearDownClass(cls):
        for workspace in cls.instruments.values():
            DeleteWorkspace(workspace)

    def tearDown(self):
        # deleting a group also deletes its members
        for name in mtd.getObjectNames():
            if not name.startswith("__") and name in mtd:
                DeleteWorkspace(name)

    @classmethod
    def _create_workspace(cls, name, signal, error_squared, instrument="WAND", logs=None):
        """
        MDHistoWorkspace with dimensions (y, x, scanIndex), an instrument, and the given sample logs.

        :param signal: array of shape (y, x, scanIndex).
        :param instrument: "WAND", "DEMAND", or None for a workspace without ExperimentInfo.
        :param logs: dictionary of log name to list of values.
        """
        shape = signal.shape
        workspace = CreateMDHistoWorkspace(
            Dimensionality=len(shape),
            Extents=",".join(f"0.5,{n + 0.5}" for n in shape),
            SignalInput=signal.ravel(order="F"),
            ErrorInput=np.sqrt(error_squared).ravel(order="F"),
            NumberOfBins=",".join(str(n) for n in shape),
            Names=",".join(["y", "x", "scanIndex"][: len(shape)]),
            Units=",".join(["bin"] * len(shape)),
            OutputWorkspace=name,
        )
        if instrument is not None:
            experiment_info = CloneWorkspace(cls.instruments[instrument], OutputWorkspace="__experiment_info")
            for log_name, values in (logs or {}).items():
                experiment_info.mutableRun().addProperty(log_name, values, True)
            workspace.addExperimentInfo(experiment_info)
            DeleteWorkspace(experiment_info)
        return workspace

    def _create_sample(self, instrument="WAND", normalized_by=None, flux=None, name="sample"):
        logs = {self.FLUX_LOGS[instrument][normalized_by]: flux or self.SAMPLE_FLUX} if normalized_by else None
        return self._create_workspace(name, self.SAMPLE_SIGNAL, self.SAMPLE_ERROR_SQUARED, instrument, logs)

    def _create_background(self, instrument="WAND", normalized_by=None, flux=None, name="background"):
        logs = {self.FLUX_LOGS[instrument][normalized_by]: flux or self.BACKGROUND_FLUX} if normalized_by else None
        return self._create_workspace(name, self.BACKGROUND_SIGNAL, self.BACKGROUND_ERROR_SQUARED, instrument, logs)

    def _assert_subtraction(self, output, flux_ratio, nullify=False):
        expected_signal = self.SAMPLE_SIGNAL - self.BACKGROUND_SIGNAL * flux_ratio
        if nullify:
            expected_signal[expected_signal < 0] = 0.0
        expected_error_squared = self.SAMPLE_ERROR_SQUARED + self.BACKGROUND_ERROR_SQUARED * flux_ratio**2
        np.testing.assert_allclose(output.getSignalArray(), expected_signal)
        np.testing.assert_allclose(output.getErrorSquaredArray(), expected_error_squared)

    def test_fixture_layout(self):
        sample = self._create_sample()
        np.testing.assert_array_equal(sample.getSignalArray(), self.SAMPLE_SIGNAL)
        self.assertEqual(sample.getDimension(2).getNBins(), 4)

    def test_subtraction_without_normalization(self):
        output = HFIRSCDSubtractBackground(
            InputWorkspace=self._create_sample(),
            InputBackground=self._create_background(),
            NullifyNegativeValues=False,
            OutputWorkspace="output",
        )
        self._assert_subtraction(output, np.ones(4))

    def test_subtraction_normalized_by_flux(self):
        for instrument in ("WAND", "DEMAND"):
            for normalized_by in ("Monitor", "Time"):
                with self.subTest(instrument=instrument, normalized_by=normalized_by):
                    output = HFIRSCDSubtractBackground(
                        InputWorkspace=self._create_sample(instrument, normalized_by),
                        InputBackground=self._create_background(instrument, normalized_by),
                        NormalizedBy=normalized_by,
                        NullifyNegativeValues=False,
                        OutputWorkspace="output",
                    )
                    self._assert_subtraction(output, self.FLUX_RATIO)

    def test_nullify_negative_values(self):
        # half the background flux doubles the flux ratio, so that some bins become negative
        flux_ratio = 2 * self.FLUX_RATIO
        self.assertTrue(np.any(self.SAMPLE_SIGNAL - self.BACKGROUND_SIGNAL * flux_ratio < 0))
        sample = self._create_sample(normalized_by="Monitor")
        background = self._create_background(normalized_by="Monitor", flux=[self.BACKGROUND_FLUX[0] / 2])
        output = HFIRSCDSubtractBackground(
            InputWorkspace=sample, InputBackground=background, NormalizedBy="Monitor", OutputWorkspace="output"
        )
        # negative bins are set to zero and keep the propagated errors
        self.assertGreaterEqual(output.getSignalArray().min(), 0.0)
        self._assert_subtraction(output, flux_ratio, nullify=True)

    def test_in_place(self):
        sample = self._create_sample()
        HFIRSCDSubtractBackground(InputWorkspace="sample", InputBackground=self._create_background(), OutputWorkspace="sample")
        self.assertEqual(mtd.getObjectNames().count("sample"), 1)
        self.assertNotIn("output", mtd)
        self._assert_subtraction(sample, np.ones(4), nullify=True)

    def test_new_output_leaves_input_unchanged(self):
        sample = self._create_sample()
        output = HFIRSCDSubtractBackground(InputWorkspace=sample, InputBackground=self._create_background(), OutputWorkspace="output")
        np.testing.assert_array_equal(sample.getSignalArray(), self.SAMPLE_SIGNAL)
        np.testing.assert_allclose(sample.getErrorSquaredArray(), self.SAMPLE_ERROR_SQUARED)
        self._assert_subtraction(output, np.ones(4), nullify=True)
        self.assertEqual(output.getExperimentInfo(0).getInstrumentName(), "WAND")

    def _assert_rejected(self, message, sample, background, **kwargs):
        with self.assertRaisesRegex(RuntimeError, message):
            HFIRSCDSubtractBackground(InputWorkspace=sample, InputBackground=background, OutputWorkspace="output", **kwargs)

    def test_validation_rejects_wrong_number_of_dimensions(self):
        two_dimensional = self._create_workspace("two_dimensional", self.SAMPLE_SIGNAL[:, :, 0], self.SAMPLE_ERROR_SQUARED[:, :, 0])
        self._assert_rejected("InputWorkspace: must have 3 dimensions", two_dimensional, self._create_background())

    def test_validation_rejects_workspace_without_instrument(self):
        background = self._create_workspace("no_instrument", self.BACKGROUND_SIGNAL, self.BACKGROUND_ERROR_SQUARED, instrument=None)
        self._assert_rejected("InputBackground: must have an instrument", self._create_sample(), background)

    def test_validation_rejects_unsupported_instrument(self):
        sample = self._create_workspace("other_instrument", self.SAMPLE_SIGNAL, self.SAMPLE_ERROR_SQUARED, instrument=None)
        sample.addExperimentInfo(CreateSampleWorkspace(NumBanks=1, BankPixelWidth=2, OutputWorkspace="other_instrument_source"))
        self._assert_rejected(
            "InputWorkspace: has instrument basic_rect; supported instruments are WAND", sample, self._create_background()
        )

    def test_validation_rejects_background_with_several_angles(self):
        background = self._create_workspace("several_angles", self.SAMPLE_SIGNAL, self.SAMPLE_ERROR_SQUARED)
        self._assert_rejected("InputBackground: must have exactly one goniometer angle, found 4", self._create_sample(), background)

    def test_validation_rejects_pixel_mismatch(self):
        background = self._create_workspace("other_pixels", self.BACKGROUND_SIGNAL[:, :2, :], self.BACKGROUND_ERROR_SQUARED[:, :2, :])
        self._assert_rejected(
            r"InputWorkspace: pixel dimensions \(2, 3\) do not match those of InputBackground \(2, 2\)", self._create_sample(), background
        )

    def test_validation_rejects_instrument_mismatch(self):
        self._assert_rejected(
            "InputWorkspace: is from WAND but InputBackground is from DEMAND", self._create_sample(), self._create_background("DEMAND")
        )

    def test_validation_rejects_missing_flux_log(self):
        self._assert_rejected(
            "InputBackground: the 'monitor_count' log required by NormalizedBy=Monitor is missing",
            self._create_sample(normalized_by="Monitor"),
            self._create_background(),
            NormalizedBy="Monitor",
        )

    def test_validation_rejects_flux_log_of_wrong_size(self):
        self._assert_rejected(
            r"InputWorkspace: the 'duration' log has 2 values, expected one per goniometer angle \(4\)",
            self._create_sample(normalized_by="Time", flux=[1.0, 2.0]),
            self._create_background(normalized_by="Time"),
            NormalizedBy="Time",
        )

    def test_validation_rejects_flux_that_is_not_positive(self):
        self._assert_rejected(
            "InputBackground: the 'monitor' log must contain finite, positive values",
            self._create_sample("DEMAND", "Monitor"),
            self._create_background("DEMAND", "Monitor", flux=[0.0]),
            NormalizedBy="Monitor",
        )

    # WorkspaceGroup sample and OutputBackgroundWorkspace

    # the two members of the group sample differ by their flux, hence by their flux ratio
    MEMBER_FLUX = {"scan_1": [100.0, 200.0, 300.0, 400.0], "scan_2": [50.0, 50.0, 100.0, 100.0]}

    def _create_group(self, name="scans"):
        for member_name, flux in self.MEMBER_FLUX.items():
            self._create_sample(normalized_by="Monitor", flux=flux, name=member_name)
        return GroupWorkspaces(InputWorkspaces=list(self.MEMBER_FLUX), OutputWorkspace=name)

    def _member_flux_ratio(self, member_name):
        return np.array(self.MEMBER_FLUX[member_name]) / self.BACKGROUND_FLUX[0]

    def _assert_scaled_background(self, scaled_background, flux_ratio):
        np.testing.assert_allclose(scaled_background.getSignalArray(), self.BACKGROUND_SIGNAL * flux_ratio)
        np.testing.assert_allclose(scaled_background.getErrorSquaredArray(), self.BACKGROUND_ERROR_SQUARED * flux_ratio**2)

    def test_group_in_place(self):
        group = self._create_group()
        HFIRSCDSubtractBackground(
            InputWorkspace="scans",
            InputBackground=self._create_background(normalized_by="Monitor"),
            NormalizedBy="Monitor",
            NullifyNegativeValues=False,
            OutputWorkspace="scans",
        )
        self.assertEqual(sorted(mtd.getObjectNames()), ["background", "scan_1", "scan_2", "scans"])
        self.assertEqual(list(mtd["scans"].getNames()), list(group.getNames()))
        for member_name in self.MEMBER_FLUX:
            self._assert_subtraction(mtd[member_name], self._member_flux_ratio(member_name))

    def test_group_new_output(self):
        self._create_group()
        output = HFIRSCDSubtractBackground(
            InputWorkspace="scans",
            InputBackground=self._create_background(normalized_by="Monitor"),
            NormalizedBy="Monitor",
            NullifyNegativeValues=False,
            OutputWorkspace="output",
        )
        self.assertIsInstance(output, WorkspaceGroup)
        self.assertEqual(sorted(output.getNames()), ["output_scan_1", "output_scan_2"])
        for member_name in self.MEMBER_FLUX:
            np.testing.assert_array_equal(mtd[member_name].getSignalArray(), self.SAMPLE_SIGNAL)
            self._assert_subtraction(mtd[f"output_{member_name}"], self._member_flux_ratio(member_name))
            self.assertEqual(mtd[f"output_{member_name}"].getExperimentInfo(0).getInstrumentName(), "WAND")

    def test_group_in_place_with_background_as_member(self):
        # the background, subtracted first, must not change the background subtracted from the next member
        self._create_background()
        sample = self._create_sample()
        GroupWorkspaces(InputWorkspaces=["background", "sample"], OutputWorkspace="scans")
        HFIRSCDSubtractBackground(
            InputWorkspace="scans", InputBackground="background", NullifyNegativeValues=False, OutputWorkspace="scans"
        )
        np.testing.assert_array_equal(mtd["background"].getSignalArray(), np.zeros(self.BACKGROUND_SIGNAL.shape))
        self._assert_subtraction(sample, np.ones(4))

    def test_output_background_workspace(self):
        output, scaled_background = HFIRSCDSubtractBackground(
            InputWorkspace=self._create_sample(normalized_by="Monitor"),
            InputBackground=self._create_background(normalized_by="Monitor"),
            NormalizedBy="Monitor",
            OutputWorkspace="output",
            OutputBackgroundWorkspace="scaled_background",
        )
        self._assert_scaled_background(scaled_background, self.FLUX_RATIO)
        # the scaled background has the dimensions and the logs of the sample
        self.assertEqual(scaled_background.getDimension(2).getNBins(), 4)
        np.testing.assert_allclose(scaled_background.getExperimentInfo(0).run().getProperty("monitor_count").value, self.SAMPLE_FLUX)

    def test_output_background_workspace_for_group(self):
        self._create_group()
        HFIRSCDSubtractBackground(
            InputWorkspace="scans",
            InputBackground=self._create_background(normalized_by="Monitor"),
            NormalizedBy="Monitor",
            OutputWorkspace="scans",
            OutputBackgroundWorkspace="scaled_background",
        )
        self.assertIsInstance(mtd["scaled_background"], WorkspaceGroup)
        self.assertEqual(sorted(mtd["scaled_background"].getNames()), ["scaled_background_scan_1", "scaled_background_scan_2"])
        for member_name in self.MEMBER_FLUX:
            self._assert_scaled_background(mtd[f"scaled_background_{member_name}"], self._member_flux_ratio(member_name))

    def test_no_background_workspace_unless_requested(self):
        self._create_group()
        HFIRSCDSubtractBackground(InputWorkspace="scans", InputBackground=self._create_background(), OutputWorkspace="output")
        self.assertEqual(
            sorted(mtd.getObjectNames()),
            [
                "background",
                "output",
                "output_scan_1",
                "output_scan_2",
                "scan_1",
                "scan_2",
                "scans",
            ],
        )

    def test_validation_names_the_invalid_members(self):
        self._create_sample(name="scan_1")
        self._create_sample("DEMAND", name="scan_2")
        CreateSampleWorkspace(NumBanks=1, BankPixelWidth=2, OutputWorkspace="scan_3")
        GroupWorkspaces(InputWorkspaces=["scan_1", "scan_2", "scan_3"], OutputWorkspace="scans")
        self._assert_rejected(
            "Member 'scan_2': is from DEMAND but InputBackground is from WAND; Member 'scan_3': must be an MDHistoWorkspace",
            "scans",
            self._create_background(),
        )

    # InputGroupingWorkspace and Workspace2D backgrounds, on synthetic workspaces

    # ungrouped background: 4 x 6 pixels, pixel (i, j) holding detector i + 4 j, so that the detectorID log, ordered
    # with the first dimension varying fastest, is 0, 1, ..., 23
    UNGROUPED_SIGNAL = np.arange(1.0, 25.0).reshape((4, 6, 1), order="F")
    UNGROUPED_ERROR_SQUARED = UNGROUPED_SIGNAL / 10.0
    UNGROUPED_IDS = list(range(24))
    # 2x2 grouping: detector i + 4 j belongs to group (i // 2) + 2 (j // 2) + 1, so that the 2 x 3 grouped pixel
    # (row, col) holds group row + 2 col + 1, whose first detector is 2 row + 8 col
    GROUPED_FIRST_IDS = [2 * row + 8 * col for col in range(3) for row in range(2)]

    @staticmethod
    def _create_grouping_workspace(name="grouping"):
        """GroupingWorkspace without instrument, as built by LoadWANDSCD, for the 2x2 grouping of the 4 x 6 pixels."""
        grouping = WorkspaceFactory.create("GroupingWorkspace", 24, 1, 1)
        for detector_id in range(24):
            i, j = detector_id % 4, detector_id // 4
            grouping.mutableY(detector_id)[0] = float(i // 2 + 2 * (j // 2) + 1)
            grouping.getSpectrum(detector_id).setDetectorID(detector_id)
        mtd.addOrReplace(name, grouping)
        return grouping

    @staticmethod
    def _grouped(array):
        """Sum of a 4 x 6 x 1 array over 2x2 blocks of pixels, a 2 x 3 x 1 array."""
        return array.reshape((2, 2, 3, 2, 1)).sum(axis=(1, 3))

    def _create_grouped_sample(self, pixel_ids=None, name="sample"):
        logs = {"detectorID": pixel_ids or self.GROUPED_FIRST_IDS}
        return self._create_workspace(name, self.SAMPLE_SIGNAL, self.SAMPLE_ERROR_SQUARED, logs=logs)

    def _create_ungrouped_background(self, with_ids=True, name="background"):
        logs = {"detectorID": self.UNGROUPED_IDS} if with_ids else None
        return self._create_workspace(name, self.UNGROUPED_SIGNAL, self.UNGROUPED_ERROR_SQUARED, logs=logs)

    def test_grouping(self):
        output, scaled_background = HFIRSCDSubtractBackground(
            InputWorkspace=self._create_grouped_sample(),
            InputBackground=self._create_ungrouped_background(),
            InputGroupingWorkspace=self._create_grouping_workspace(),
            NullifyNegativeValues=False,
            OutputWorkspace="output",
            OutputBackgroundWorkspace="scaled_background",
        )
        grouped_signal = self._grouped(self.UNGROUPED_SIGNAL)
        grouped_error_squared = self._grouped(self.UNGROUPED_ERROR_SQUARED)
        np.testing.assert_allclose(scaled_background.getSignalArray(), np.repeat(grouped_signal, 4, axis=2))
        np.testing.assert_allclose(output.getSignalArray(), self.SAMPLE_SIGNAL - grouped_signal)
        np.testing.assert_allclose(output.getErrorSquaredArray(), self.SAMPLE_ERROR_SQUARED + grouped_error_squared)

    def _assert_grouping_rejected(self, message, sample, background):
        self._assert_rejected(message, sample, background, InputGroupingWorkspace=self._create_grouping_workspace())

    def test_validation_rejects_grouping_that_is_not_a_grouping_workspace(self):
        CreateSampleWorkspace(NumBanks=1, BankPixelWidth=2, OutputWorkspace="not_grouping")
        self._assert_rejected(
            "InputGroupingWorkspace: must be a GroupingWorkspace",
            self._create_grouped_sample(),
            self._create_ungrouped_background(),
            InputGroupingWorkspace="not_grouping",
        )

    def test_validation_rejects_grouped_background_with_grouping(self):
        self._assert_grouping_rejected(
            r"InputBackground: must not be grouped when InputGroupingWorkspace is provided: it has 6 pixels, but an "
            r"ungrouped background has one pixel per spectrum of InputGroupingWorkspace \(24\). If InputBackground was "
            "loaded with the same grouping as InputWorkspace, do not provide InputGroupingWorkspace",
            self._create_grouped_sample(),
            self._create_background(),
        )

    def test_validation_rejects_finely_grouped_background_with_grouping(self):
        # 4 x 3 pixels: the 4 x 6 detectors grouped in pairs along the second dimension, finer than the 2x2 grouping
        background = self._create_workspace(
            "finely_grouped",
            self.UNGROUPED_SIGNAL[:, ::2, :] + self.UNGROUPED_SIGNAL[:, 1::2, :],
            self.UNGROUPED_ERROR_SQUARED[:, ::2, :] + self.UNGROUPED_ERROR_SQUARED[:, 1::2, :],
            logs={"detectorID": [i + 8 * j for j in range(3) for i in range(4)]},
        )
        self._assert_grouping_rejected(
            "InputBackground: must not be grouped when InputGroupingWorkspace is provided: it has 12 pixels",
            self._create_grouped_sample(),
            background,
        )

    def test_validation_rejects_workspace2d_with_other_spectra_than_grouping(self):
        self._assert_grouping_rejected(
            r"InputBackground: must have one spectrum per spectrum of InputGroupingWorkspace \(24\), found 1966080",
            self._create_grouped_sample(),
            self.instruments["WAND"],
        )

    def test_validation_rejects_background_without_detector_ids_with_grouping(self):
        self._assert_grouping_rejected(
            "InputBackground: the 'detectorID' log, required with InputGroupingWorkspace, is missing",
            self._create_grouped_sample(),
            self._create_ungrouped_background(with_ids=False),
        )

    def test_validation_rejects_sample_without_detector_ids_with_grouping(self):
        self._assert_grouping_rejected(
            "InputWorkspace: the 'detectorID' log, required with InputGroupingWorkspace, is missing",
            self._create_sample(),
            self._create_ungrouped_background(),
        )

    def test_validation_rejects_sample_with_other_number_of_groups(self):
        sample = self._create_workspace("other_grouping", self.SAMPLE_SIGNAL[:, :2, :], self.SAMPLE_ERROR_SQUARED[:, :2, :])
        self._assert_grouping_rejected(
            "InputWorkspace: has 4 pixels but InputGroupingWorkspace has 6 groups", sample, self._create_ungrouped_background()
        )

    def test_detector_id_missing_from_grouping_raises(self):
        with self.assertRaisesRegex(RuntimeError, "1 of 6 detector IDs are not found in InputGroupingWorkspace"):
            HFIRSCDSubtractBackground(
                InputWorkspace=self._create_grouped_sample(pixel_ids=self.GROUPED_FIRST_IDS[:5] + [999]),
                InputBackground=self._create_ungrouped_background(),
                InputGroupingWorkspace=self._create_grouping_workspace(),
                OutputWorkspace="output",
            )

    def test_validation_rejects_workspace2d_with_several_bins(self):
        background = CreateSampleWorkspace(NumBanks=1, BankPixelWidth=2, OutputWorkspace="several_bins")
        self._assert_rejected(
            f"InputBackground: a Workspace2D background must have one value per spectrum, found {background.blocksize()}",
            self._create_sample(),
            background,
        )

    def test_validation_rejects_workspace2d_of_unsupported_instrument(self):
        background = CreateSampleWorkspace(NumBanks=1, BankPixelWidth=2, XMax=1.0, BinWidth=1.0, OutputWorkspace="one_bin")
        self._assert_rejected(
            "InputBackground: has instrument basic_rect; supported instruments are WAND", self._create_sample(), background
        )

    def test_validation_rejects_grouped_sample_with_workspace2d_and_no_grouping(self):
        self._assert_rejected(
            "InputWorkspace: has 6 pixels but InputBackground has 1966080 spectra; if InputWorkspace was grouped, "
            "provide InputGroupingWorkspace",
            self._create_grouped_sample(),
            self.instruments["WAND"],
        )

    def test_validation_hints_at_grouping_on_pixel_mismatch(self):
        self._assert_rejected(
            "if InputWorkspace was grouped and InputBackground was not, provide InputGroupingWorkspace",
            self._create_grouped_sample(),
            self._create_ungrouped_background(),
        )


if __name__ == "__main__":
    unittest.main()
