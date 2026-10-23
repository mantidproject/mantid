# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2026 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import unittest

import numpy as np

from mantid.api import mtd
from mantid.simpleapi import (
    CloneWorkspace,
    CreateMDHistoWorkspace,
    CreateSampleWorkspace,
    DeleteWorkspace,
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
        for name in mtd.getObjectNames():
            if not name.startswith("__"):
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
        self._assert_rejected("InputWorkspace must have 3 dimensions", two_dimensional, self._create_background())

    def test_validation_rejects_workspace_without_instrument(self):
        background = self._create_workspace("no_instrument", self.BACKGROUND_SIGNAL, self.BACKGROUND_ERROR_SQUARED, instrument=None)
        self._assert_rejected("InputBackground must have an instrument", self._create_sample(), background)

    def test_validation_rejects_unsupported_instrument(self):
        sample = self._create_workspace("other_instrument", self.SAMPLE_SIGNAL, self.SAMPLE_ERROR_SQUARED, instrument=None)
        sample.addExperimentInfo(CreateSampleWorkspace(NumBanks=1, BankPixelWidth=2, OutputWorkspace="other_instrument_source"))
        self._assert_rejected("supported instruments are WAND", sample, self._create_background())

    def test_validation_rejects_background_with_several_angles(self):
        background = self._create_workspace("several_angles", self.SAMPLE_SIGNAL, self.SAMPLE_ERROR_SQUARED)
        self._assert_rejected("InputBackground must have exactly one goniometer angle, found 4", self._create_sample(), background)

    def test_validation_rejects_pixel_mismatch(self):
        background = self._create_workspace("other_pixels", self.BACKGROUND_SIGNAL[:, :2, :], self.BACKGROUND_ERROR_SQUARED[:, :2, :])
        self._assert_rejected(r"pixel dimensions of InputBackground \(2, 2\) do not match", self._create_sample(), background)

    def test_validation_rejects_instrument_mismatch(self):
        self._assert_rejected(
            "InputBackground is from DEMAND but InputWorkspace is from WAND", self._create_sample(), self._create_background("DEMAND")
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


if __name__ == "__main__":
    unittest.main()
