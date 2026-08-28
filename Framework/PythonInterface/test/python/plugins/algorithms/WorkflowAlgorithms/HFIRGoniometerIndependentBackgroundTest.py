# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2023 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +

import unittest
from mantid.simpleapi import (
    CreateMDHistoWorkspace,
    DeleteWorkspace,
    HFIRGoniometerIndependentBackground,
    LoadEmptyInstrument,
)
import numpy as np
import scipy.ndimage
import scipy.stats


class HFIRGoniometerIndependentBackgroundTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.instrument = LoadEmptyInstrument(InstrumentName="HB2C", OutputWorkspace="instrument")

    @classmethod
    def tearDownClass(cls):
        DeleteWorkspace(cls.instrument)

    def setUp(self):
        self.signal = np.random.default_rng(12345).random((100, 100, 100))
        _max = self.signal.max()
        _min = self.signal.min()

        self.workspace = CreateMDHistoWorkspace(
            SignalInput=self.signal,
            ErrorInput=np.ones_like(self.signal),
            Dimensionality=3,
            Extents=f"{_min},{_max},{_min},{_max},{_min},{_max}",
            Names="x,y,z",
            NumberOfBins="100,100,100",
            Units="number,number,number",
            OutputWorkspace="input",
        )
        self.duration = np.arange(1.0, 101.0)
        self.monitor_count = np.arange(101.0, 201.0)
        self.workspace.addExperimentInfo(self.instrument)
        run = self.workspace.getExperimentInfo(0).run()
        run.addProperty("duration", self.duration.tolist(), True)
        run.addProperty("monitor_count", self.monitor_count.tolist(), True)

        self.demand_workspace = CreateMDHistoWorkspace(
            SignalInput=self.signal,
            ErrorInput=np.ones_like(self.signal),
            Dimensionality=3,
            Extents=f"{_min},{_max},{_min},{_max},{_min},{_max}",
            Names="x,y,z",
            NumberOfBins="100,100,100",
            Units="number,number,number",
            OutputWorkspace="demand_input",
        )
        demand_instrument = LoadEmptyInstrument(InstrumentName="HB3A", OutputWorkspace="demand_instrument")
        self.demand_workspace.addExperimentInfo(demand_instrument)
        demand_run = self.demand_workspace.getExperimentInfo(0).run()
        demand_run.addProperty("time", self.duration.tolist(), True)
        demand_run.addProperty("monitor", self.monitor_count.tolist(), True)

    def tearDown(self):
        DeleteWorkspace(self.workspace)
        DeleteWorkspace(self.demand_workspace)
        DeleteWorkspace("demand_instrument")

    def test_generate_background_pf(self):
        signal = self.workspace.getSignalArray().copy()
        expected = scipy.ndimage.percentile_filter(signal, 50, size=(1, 1, 25), mode="nearest")

        outputWS = HFIRGoniometerIndependentBackground(self.workspace, BackgroundLevel=50, BackgroundWindowSize=25)
        result = outputWS.getSignalArray().copy()
        self.assertTrue(np.array_equal(expected, result))

    def test_time_normalization_is_applied_before_windowed_percentile(self):
        """Verify that rotation durations normalize the data before windowed percentile filtering."""
        signal = self.workspace.getSignalArray().copy()
        normalized_signal = signal / self.duration[np.newaxis, np.newaxis, :]
        expected = scipy.ndimage.percentile_filter(normalized_signal, 50, size=(1, 1, 25), mode="nearest")

        outputWS = HFIRGoniometerIndependentBackground(
            self.workspace, BackgroundLevel=50, BackgroundWindowSize=25, NormalizeBy="Time", NormalizeOutput=True
        )

        np.testing.assert_array_equal(outputWS.getSignalArray(), expected)

    def test_monitor_normalization_can_be_restored_per_rotation(self):
        """Verify that monitor-normalized global backgrounds can be restored to each rotation's scale."""
        signal = self.workspace.getSignalArray().copy()
        normalized_signal = signal / self.monitor_count[np.newaxis, np.newaxis, :]
        normalized_expected = np.percentile(normalized_signal, 50, axis=2)
        expected = np.repeat(normalized_expected[:, :, np.newaxis], signal.shape[2], axis=2)
        expected *= self.monitor_count[np.newaxis, np.newaxis, :]

        outputWS = HFIRGoniometerIndependentBackground(self.workspace, NormalizeBy="Monitor", NormalizeOutput=False)

        np.testing.assert_allclose(outputWS.getSignalArray(), expected)

    def test_demand_time_log_is_used_for_time_normalization(self):
        outputWS = HFIRGoniometerIndependentBackground(self.demand_workspace, NormalizeBy="Time", NormalizeOutput=True)

        signal = self.demand_workspace.getSignalArray()
        expected = np.percentile(signal / self.duration[np.newaxis, np.newaxis, :], 50, axis=2)
        expected = np.repeat(expected[:, :, np.newaxis], signal.shape[2], axis=2)
        np.testing.assert_allclose(outputWS.getSignalArray(), expected)

    def test_demand_monitor_log_is_used_for_monitor_normalization(self):
        outputWS = HFIRGoniometerIndependentBackground(self.demand_workspace, NormalizeBy="Monitor", NormalizeOutput=True)

        signal = self.demand_workspace.getSignalArray()
        expected = np.percentile(signal / self.monitor_count[np.newaxis, np.newaxis, :], 50, axis=2)
        expected = np.repeat(expected[:, :, np.newaxis], signal.shape[2], axis=2)
        np.testing.assert_allclose(outputWS.getSignalArray(), expected)

    def test_normalization_is_rejected_for_unsupported_instrument(self):
        workspace = CreateMDHistoWorkspace(
            SignalInput=[1.0],
            ErrorInput=[1.0],
            Dimensionality=3,
            Extents="0,1,0,1,0,1",
            Names="x,y,z",
            NumberOfBins="1,1,1",
            Units="number,number,number",
            OutputWorkspace="unsupported_input",
        )
        unsupported_instrument = LoadEmptyInstrument(InstrumentName="HB2A", OutputWorkspace="unsupported_instrument")
        workspace.addExperimentInfo(unsupported_instrument)
        with self.assertRaisesRegex(RuntimeError, "not supported for instrument HB2A"):
            HFIRGoniometerIndependentBackground(workspace, NormalizeBy="Time", OutputWorkspace="unsupported_output")
        DeleteWorkspace(workspace)
        DeleteWorkspace("unsupported_instrument")

    def test_invalid_normalization_factors_are_rejected(self):
        for normalize_by, log_name in (("Time", "duration"), ("Monitor", "monitor_count")):
            for invalid_factor in (0.0, -1.0, np.nan, np.inf):
                factors = np.ones_like(self.duration)
                factors[0] = invalid_factor
                self.workspace.getExperimentInfo(0).run().addProperty(log_name, factors.tolist(), True)

                with self.assertRaisesRegex(RuntimeError, "must contain finite, positive normalization factors"):
                    HFIRGoniometerIndependentBackground(self.workspace, NormalizeBy=normalize_by, OutputWorkspace="invalid_factors_output")

    def test_window_larger_than_rotation_axis_is_rejected(self):
        with self.assertRaisesRegex(TypeError, "Some invalid Properties found"):
            HFIRGoniometerIndependentBackground(self.workspace, BackgroundWindowSize=self.signal.shape[2] + 1)

    def test_normalized_output_error_is_selected_and_scaled(self):
        """Verify percentile-associated errors are retained when normalized and rescaled correctly."""
        signal = self.workspace.getSignalArray().copy()
        error_squared = self.workspace.getErrorSquaredArray().copy()
        normalized_signal = signal / self.monitor_count[np.newaxis, np.newaxis, :]
        normalized_errors = error_squared / self.monitor_count[np.newaxis, np.newaxis, :] ** 2
        expected_error = self._selected_error_squared(normalized_signal, normalized_errors, 50)
        expected_error = expected_error * self._estimator_scale(50, signal.shape[2])
        expected_error = np.repeat(expected_error[:, :, np.newaxis], signal.shape[2], axis=2)

        outputWS = HFIRGoniometerIndependentBackground(self.workspace, NormalizeBy="Monitor", NormalizeOutput=True)

        np.testing.assert_allclose(outputWS.getErrorSquaredArray(), expected_error)

        outputWS = HFIRGoniometerIndependentBackground(self.workspace, NormalizeBy="Monitor", NormalizeOutput=False)
        expected_error *= self.monitor_count[np.newaxis, np.newaxis, :] ** 2
        np.testing.assert_allclose(outputWS.getErrorSquaredArray(), expected_error)

    def test_error_is_the_estimator_uncertainty_not_the_selected_value(self):
        """Verify the output variance is the percentile estimator's, not that of the value it selected."""
        counts = np.array([12.0, 9.0, 14.0, 11.0, 350.0])
        workspace = CreateMDHistoWorkspace(
            SignalInput=counts,
            ErrorInput=np.sqrt(counts),
            Dimensionality=3,
            Extents="0,1,0,1,0,5",
            Names="x,y,z",
            NumberOfBins="1,1,5",
            Units="number,number,number",
            OutputWorkspace="single_pixel",
        )

        outputWS = HFIRGoniometerIndependentBackground(workspace)

        # Sorted counts are 9, 11, 12, 14, 350, so the median is 12 and the peak at 350 is rejected.
        np.testing.assert_allclose(outputWS.getSignalArray(), np.full((1, 1, 5), 12.0))
        # The selected value carries a Poisson variance of 12, reduced by pi/2 / 5 for a median of 5 rotations.
        np.testing.assert_allclose(outputWS.getErrorSquaredArray(), np.full((1, 1, 5), 12.0 * np.pi / 2 / 5))
        DeleteWorkspace(workspace)

    def test_windowed_error_uses_the_window_size_as_the_sample_count(self):
        """Verify the windowed branch averages over the window, not over the whole rotation axis."""
        window_size = 25

        outputWS = HFIRGoniometerIndependentBackground(self.workspace, BackgroundLevel=50, BackgroundWindowSize=window_size)

        # Every input variance is unity, so the output variance is the estimator scale factor alone.
        expected = np.full_like(self.workspace.getErrorSquaredArray(), self._estimator_scale(50, window_size))
        np.testing.assert_allclose(outputWS.getErrorSquaredArray(), expected)

    def test_extreme_percentiles_keep_the_selected_value_variance(self):
        """Verify the smallest and largest values fall back to the selected variance, where the estimator has no limit."""
        for background_level in (0.0, 100.0):
            outputWS = HFIRGoniometerIndependentBackground(self.workspace, BackgroundLevel=background_level)

            expected = np.ones_like(self.workspace.getErrorSquaredArray())
            np.testing.assert_allclose(outputWS.getErrorSquaredArray(), expected)

    def test_negative_background_level_is_rejected(self):
        """Verify a negative percentile is refused rather than silently selecting the wrong value."""
        for extra in ({}, {"BackgroundWindowSize": 25}):
            with self.assertRaises(ValueError):
                HFIRGoniometerIndependentBackground(self.workspace, BackgroundLevel=-20.0, **extra)

    @staticmethod
    def _selected_error_squared(signal, error_squared, percentile):
        """Return the error variance of the single value the percentile selects along the rotation axis."""
        order = np.argsort(signal, axis=2)
        sorted_errors = np.take_along_axis(error_squared, order, axis=2)
        position = (signal.shape[2] - 1) * percentile / 100.0
        lower = int(np.floor(position))
        upper = int(np.ceil(position))
        weight = position - lower
        return (1.0 - weight) * sorted_errors[:, :, lower] + weight * sorted_errors[:, :, upper]

    @staticmethod
    def _estimator_scale(percentile, n_samples):
        """Return the independently derived factor from a selected value's variance to the estimator's."""
        p = percentile / 100.0
        density = scipy.stats.norm.pdf(scipy.stats.norm.ppf(p))
        return p * (1.0 - p) / density**2 / n_samples

    def test_generate_background_np(self):
        signal = self.workspace.getSignalArray().copy()
        bkg = np.percentile(signal, 50, axis=2)
        expected = np.repeat(bkg[:, :, np.newaxis], signal.shape[2], axis=2)

        outputWS = HFIRGoniometerIndependentBackground(self.workspace)
        result = outputWS.getSignalArray().copy()
        self.assertTrue(np.array_equal(expected, result))


if __name__ == "__main__":
    unittest.main()
