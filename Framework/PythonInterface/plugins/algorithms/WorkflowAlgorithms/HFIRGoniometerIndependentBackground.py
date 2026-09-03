# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2023 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +

from mantid.api import PythonAlgorithm, AlgorithmFactory, WorkspaceProperty, IMDHistoWorkspaceProperty
from mantid.kernel import Direction, FloatBoundedValidator, IntBoundedValidator, StringListValidator, Property
from mantid.simpleapi import CloneWorkspace

import numpy as np
from scipy.stats import norm


class HFIRGoniometerIndependentBackground(PythonAlgorithm):
    _NORMALIZATION_LOGS = {
        "WAND": {"Time": "duration", "Monitor": "monitor_count"},
        "HB2C": {"Time": "duration", "Monitor": "monitor_count"},
        "HB3A": {"Time": "time", "Monitor": "monitor"},
        "DEMAND": {"Time": "time", "Monitor": "monitor"},
    }

    def category(self):
        return "Diffraction\\Reduction;Diffraction\\Utility"

    def summary(self):
        return "Generates a background from a 3 dimensional MDHistoWorkspace."

    def version(self):
        return 1

    def PyInit(self):
        self.declareProperty(
            IMDHistoWorkspaceProperty("InputWorkspace", "", direction=Direction.Input),
            doc="Input workspace, must be a 3 dimensional MDHistoWorkspace",
        )
        self.declareProperty(
            name="BackgroundLevel",
            defaultValue=50.0,
            direction=Direction.Input,
            doc="Percentile in the range 0 to 100 defining the background level (default 50, median filter)",
            validator=FloatBoundedValidator(0.0, 100.0),
        )
        self.declareProperty(
            name="BackgroundWindowSize",
            defaultValue=Property.EMPTY_INT,
            direction=Direction.Input,
            doc="Background window size, only applies to the rotation axis, assumes the detectors are already \
              grouped. Leave unset to use every value along the rotation axis",
            validator=IntBoundedValidator(lower=1),
        )
        self.declareProperty(
            name="FilterMode",
            defaultValue="nearest",
            validator=StringListValidator(["nearest", "wrap"]),
            doc="Mode should be 'nearest' if the rotation is incomplete or 'wrap' if complete within a reasonable tolerance",
        )
        self.declareProperty(
            name="NormalizeBy",
            defaultValue="None",
            validator=StringListValidator(["None", "Time", "Monitor"]),
            doc="Normalize the input by per-rotation duration or monitor count before calculating the background.",
        )
        self.declareProperty(
            name="NormalizeOutput",
            defaultValue=False,
            doc="Keep the output normalized when True; otherwise multiply the calculated normalized "
            "background by the duration or monitor count of each rotation",
        )
        self.declareProperty(WorkspaceProperty(name="OutputWorkspace", defaultValue="", direction=Direction.Output))

    def validateInputs(self):
        issues = dict()

        inWS = self.getProperty("InputWorkspace").value

        if inWS.getNumDims() != 3:
            issues["InputWorkspace"] = "InputWorkspace has wrong number of dimensions, need 3"

        bkg_size = self.getProperty("BackgroundWindowSize").value
        if inWS.getNumDims() == 3 and bkg_size != Property.EMPTY_INT and bkg_size > inWS.getSignalArray().shape[2]:
            issues["BackgroundWindowSize"] = "BackgroundWindowSize cannot be larger than the number of rotations in InputWorkspace"

        return issues

    @classmethod
    def _get_normalization_factors(cls, data_ws, normalize_by, n_rotations):
        if normalize_by == "None":
            return np.ones(n_rotations)

        instrument_name = data_ws.getExperimentInfo(0).getInstrumentName()
        try:
            log_name = cls._NORMALIZATION_LOGS[instrument_name][normalize_by]
        except KeyError:
            raise ValueError(
                f"Normalization by {normalize_by} is not supported for instrument {instrument_name}. "
                "Supported instruments are WAND/HB2C and DEMAND/HB3A."
            ) from None

        try:
            factors = np.asarray(data_ws.getExperimentInfo(0).run().getProperty(log_name).value, dtype=float)
        except RuntimeError as error:
            raise ValueError(
                f"Required normalization log '{log_name}' for {normalize_by} normalization is missing from instrument {instrument_name}."
            ) from error
        if factors.size != n_rotations:
            raise ValueError(f"The {log_name} log has {factors.size} values, but the workspace has {n_rotations} rotations.")
        if np.any(~np.isfinite(factors)) or np.any(factors <= 0):
            raise ValueError(f"The {log_name} log must contain finite, positive normalization factors.")
        return factors

    @staticmethod
    def _global_percentile(signal, error_squared, percentile):
        """Return a percentile and the error variance of the value it selected."""
        position = (signal.shape[2] - 1) * percentile / 100.0
        lower = int(np.floor(position))
        upper = int(np.ceil(position))
        weight = position - lower

        value = np.empty(signal.shape[:2], dtype=signal.dtype)
        error = np.empty(error_squared.shape[:2], dtype=error_squared.dtype)

        # Process one detector row at a time, and rank only the two values the percentile interpolates
        # between, to avoid sorting the whole rotation axis of every detector at once.
        for row in range(signal.shape[0]):
            order = np.argpartition(signal[row], (lower, upper), axis=-1)[:, [lower, upper]]
            ranked_signal = np.take_along_axis(signal[row], order, axis=-1)
            ranked_error_squared = np.take_along_axis(error_squared[row], order, axis=-1)
            value[row] = (1.0 - weight) * ranked_signal[:, 0] + weight * ranked_signal[:, 1]
            error[row] = (1.0 - weight) * ranked_error_squared[:, 0] + weight * ranked_error_squared[:, 1]
        return value, error

    @staticmethod
    def _window_rank(percentile, window_size):
        """Return the rank a percentile selects within a window.

        scipy.ndimage.percentile_filter selects a ranked value rather than linearly interpolating it, so the
        windowed branch reports the value at this rank rather than the requested percentile itself.
        """
        return min(int(window_size * percentile / 100.0), window_size - 1)

    @classmethod
    def _windowed_percentile(cls, signal, error_squared, percentile, window_size, filter_mode):
        """Return windowed percentiles and the error variance of each value they selected."""
        pad_before = window_size // 2
        pad_after = window_size - 1 - pad_before
        mode = "edge" if filter_mode == "nearest" else "wrap"
        rank = cls._window_rank(percentile, window_size)
        selected = np.empty_like(signal)
        selected_error = np.empty_like(error_squared)

        # Process one detector row at a time to avoid materializing all detector/window combinations.
        for row in range(signal.shape[0]):
            padded_signal = np.pad(signal[row], ((0, 0), (pad_before, pad_after)), mode=mode)
            padded_error_squared = np.pad(error_squared[row], ((0, 0), (pad_before, pad_after)), mode=mode)
            windows = np.lib.stride_tricks.sliding_window_view(padded_signal, window_size, axis=1)
            error_windows = np.lib.stride_tricks.sliding_window_view(padded_error_squared, window_size, axis=1)
            order = np.argsort(windows, axis=2)
            selected[row] = np.take_along_axis(windows, order[:, :, rank : rank + 1], axis=2)[..., 0]
            selected_error[row] = np.take_along_axis(error_windows, order[:, :, rank : rank + 1], axis=2)[..., 0]
        return selected, selected_error

    @staticmethod
    def _estimator_variance_scale(percentile, n_samples, rank=None):
        """Return the factor converting a selected value's variance into the percentile estimator's variance.

        The percentile is not a measurement but an estimator built from ``n_samples`` rotation steps, so it is
        more precise than the single value it selects. For samples of standard deviation ``sigma`` drawn from a
        distribution that is locally normal around the percentile, the estimator's variance is
        ``p (1 - p) / phi(z_p)**2 * sigma**2 / n_samples``, where ``z_p`` is the standard normal quantile at
        ``p`` and ``phi`` its density. The leading factor is pi/2 for the median.

        ``rank`` is the position selected within a window of ``n_samples`` values, and is given for the
        windowed branch only. That branch reports the value at a fixed rank instead of interpolating at the
        requested percentile, so the estimator's precision follows the percentile that rank represents.

        The asymptotic result does not hold for the smallest or largest value, or for a single sample, so those
        cases fall back to the variance of the selected value itself, which is a conservative upper bound.
        """
        if rank is not None:
            if rank == 0 or rank == n_samples - 1:
                return 1.0
            percentile = 100.0 * (rank + 0.5) / n_samples
        p = percentile / 100.0
        if not 0.0 < p < 1.0 or n_samples < 2:
            return 1.0
        density = norm.pdf(norm.ppf(p))
        density_squared = density**2
        if density_squared == 0.0:
            return 1.0
        return p * (1.0 - p) / density_squared / n_samples

    def PyExec(self):
        data_ws = self.getProperty("InputWorkspace").value
        signal = data_ws.getSignalArray().copy()
        error_squared = data_ws.getErrorSquaredArray().copy()

        bkg_level = self.getProperty("BackgroundLevel").value
        bkg_size = self.getProperty("BackgroundWindowSize").value
        filter_mode = self.getProperty("FilterMode").value
        normalize_by = self.getProperty("NormalizeBy").value
        normalize_output = self.getProperty("NormalizeOutput").value
        factors = self._get_normalization_factors(data_ws, normalize_by, signal.shape[2])

        signal /= factors[np.newaxis, np.newaxis, :]
        error_squared /= factors[np.newaxis, np.newaxis, :] ** 2

        if bkg_size == Property.EMPTY_INT:
            n_samples = signal.shape[2]
            rank = None
            percent, percent_error_squared = self._global_percentile(signal, error_squared, bkg_level)
            bkg = np.repeat(percent[:, :, np.newaxis], n_samples, axis=2)
            bkg_error_squared = np.repeat(percent_error_squared[:, :, np.newaxis], n_samples, axis=2)
        else:
            n_samples = bkg_size
            rank = self._window_rank(bkg_level, bkg_size)
            bkg, bkg_error_squared = self._windowed_percentile(signal, error_squared, bkg_level, bkg_size, filter_mode)

        # The selected value's variance is the uncertainty of a single measurement. Rescale it to the
        # uncertainty of the percentile estimator, which averages over n_samples rotation steps. Taking the
        # single-measurement variance at the percentile rather than across the whole window keeps the estimate
        # free of the Bragg peaks that the percentile is chosen to reject.
        bkg_error_squared *= self._estimator_variance_scale(bkg_level, n_samples, rank)

        if not normalize_output:
            bkg *= factors[np.newaxis, np.newaxis, :]
            bkg_error_squared *= factors[np.newaxis, np.newaxis, :] ** 2

        outputWS = self.getPropertyValue("OutputWorkspace")
        outputWS = CloneWorkspace(InputWorkspace=data_ws, OutputWorkspace=outputWS)
        outputWS.setSignalArray(bkg)
        outputWS.setErrorSquaredArray(bkg_error_squared)

        self.setProperty("OutputWorkspace", outputWS)


# Register algorithm with Mantid
AlgorithmFactory.subscribe(HFIRGoniometerIndependentBackground)
