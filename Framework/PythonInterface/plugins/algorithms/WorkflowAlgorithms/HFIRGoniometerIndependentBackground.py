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


class HFIRGoniometerIndependentBackground(PythonAlgorithm):
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
            doc="Backgound level defines percentile range, (default 50, median filter)",
            validator=FloatBoundedValidator(-100.0, 100.0),
        )
        self.declareProperty(
            name="BackgroundWindowSize",
            defaultValue=Property.EMPTY_INT,
            direction=Direction.Input,
            doc="Background Window Size, only applies to the rotation axis, assumes the detectors are already \
              grouped. Integer value or -1 for All values",
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

        return issues

    @staticmethod
    def _get_normalization_factors(data_ws, normalize_by, n_rotations):
        if normalize_by == "None":
            return np.ones(n_rotations)

        log_name = {"Time": "duration", "Monitor": "monitor_count"}[normalize_by]
        factors = np.asarray(data_ws.getExperimentInfo(0).run().getProperty(log_name).value, dtype=float)
        if factors.size != n_rotations:
            raise ValueError(f"The {log_name} log has {factors.size} values, but the workspace has {n_rotations} rotations.")
        return factors

    @staticmethod
    def _global_percentile(signal, error_squared, percentile):
        """Return a percentile and its paired error variance along the rotation axis."""
        order = np.argsort(signal, axis=2)
        sorted_signal = np.take_along_axis(signal, order, axis=2)
        sorted_error_squared = np.take_along_axis(error_squared, order, axis=2)

        position = (signal.shape[2] - 1) * percentile / 100.0
        lower = int(np.floor(position))
        upper = int(np.ceil(position))
        weight = position - lower

        value = (1.0 - weight) * sorted_signal[:, :, lower] + weight * sorted_signal[:, :, upper]
        error = (1.0 - weight) * sorted_error_squared[:, :, lower] + weight * sorted_error_squared[:, :, upper]
        return value, error

    @staticmethod
    def _windowed_percentile(signal, error_squared, percentile, window_size, filter_mode):
        """Return windowed percentiles and paired error variances along the rotation axis."""
        pad_before = window_size // 2
        pad_after = window_size - 1 - pad_before
        mode = "edge" if filter_mode == "nearest" else "wrap"
        # scipy.ndimage.percentile_filter selects a ranked value rather than linearly interpolating it.
        rank = min(int(window_size * percentile / 100.0), window_size - 1)
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
            percent, percent_error_squared = self._global_percentile(signal, error_squared, bkg_level)
            bkg = np.repeat(percent[:, :, np.newaxis], signal.shape[2], axis=2)
            bkg_error_squared = np.repeat(percent_error_squared[:, :, np.newaxis], signal.shape[2], axis=2)
        else:
            bkg, bkg_error_squared = self._windowed_percentile(signal, error_squared, bkg_level, bkg_size, filter_mode)

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
