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

    def tearDown(self):
        DeleteWorkspace(self.workspace)

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

    def test_normalized_output_error_is_selected_and_scaled(self):
        """Verify percentile-associated errors are retained when normalized and rescaled correctly."""
        signal = self.workspace.getSignalArray().copy()
        error_squared = self.workspace.getErrorSquaredArray().copy()
        normalized_signal = signal / self.monitor_count[np.newaxis, np.newaxis, :]
        normalized_errors = error_squared / self.monitor_count[np.newaxis, np.newaxis, :] ** 2
        _, expected_error = self._global_percentile(normalized_signal, normalized_errors, 50)
        expected_error = np.repeat(expected_error[:, :, np.newaxis], signal.shape[2], axis=2)

        outputWS = HFIRGoniometerIndependentBackground(self.workspace, NormalizeBy="Monitor", NormalizeOutput=True)

        np.testing.assert_allclose(outputWS.getErrorSquaredArray(), expected_error)

        outputWS = HFIRGoniometerIndependentBackground(self.workspace, NormalizeBy="Monitor", NormalizeOutput=False)
        expected_error *= self.monitor_count[np.newaxis, np.newaxis, :] ** 2
        np.testing.assert_allclose(outputWS.getErrorSquaredArray(), expected_error)

    @staticmethod
    def _global_percentile(signal, error_squared, percentile):
        order = np.argsort(signal, axis=2)
        sorted_errors = np.take_along_axis(error_squared, order, axis=2)
        position = (signal.shape[2] - 1) * percentile / 100.0
        lower = int(np.floor(position))
        upper = int(np.ceil(position))
        weight = position - lower
        return None, (1.0 - weight) * sorted_errors[:, :, lower] + weight * sorted_errors[:, :, upper]

    def test_generate_background_np(self):
        signal = self.workspace.getSignalArray().copy()
        bkg = np.percentile(signal, 50, axis=2)
        expected = np.repeat(bkg[:, :, np.newaxis], signal.shape[2], axis=2)

        outputWS = HFIRGoniometerIndependentBackground(self.workspace)
        result = outputWS.getSignalArray().copy()
        self.assertTrue(np.array_equal(expected, result))


if __name__ == "__main__":
    unittest.main()
