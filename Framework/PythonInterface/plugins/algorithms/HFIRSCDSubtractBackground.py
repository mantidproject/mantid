# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2026 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from typing import Dict, Optional

import numpy as np

from mantid.api import AlgorithmFactory, IMDHistoWorkspace, IMDHistoWorkspaceProperty, PythonAlgorithm
from mantid.kernel import Direction, StringListValidator


class HFIRSCDSubtractBackground(PythonAlgorithm):
    # Instrument names returned by getInstrumentName(), mapped to the instrument they designate
    _INSTRUMENTS = {"WAND": "WAND", "HB2C": "WAND", "HB3A": "DEMAND", "DEMAND": "DEMAND"}
    # Sample logs holding the integrated flux of each goniometer angle, as in HFIRGoniometerIndependentBackground
    _FLUX_LOGS = {
        "WAND": {"Time": "duration", "Monitor": "monitor_count"},
        "DEMAND": {"Time": "time", "Monitor": "monitor"},
    }

    def category(self):
        return "Diffraction\\Reduction"

    def seeAlso(self):
        return ["LoadWANDSCD", "HB3AAdjustSampleNorm", "HFIRGoniometerIndependentBackground", "ConvertHFIRSCDtoMDE", "MDNorm"]

    def name(self):
        return "HFIRSCDSubtractBackground"

    def summary(self):
        return (
            "Subtract from WAND or DEMAND data in detector space a background measured at a single goniometer angle, "
            "scaled to the integrated flux of each goniometer angle of the sample."
        )

    def PyInit(self):
        self.declareProperty(
            IMDHistoWorkspaceProperty("InputWorkspace", "", direction=Direction.Input),
            doc="Unnormalized sample in detector space, with dimensions (y, x, goniometer angle), "
            "as returned by LoadWANDSCD or HB3AAdjustSampleNorm. The sample must be in counts.",
        )
        self.declareProperty(
            IMDHistoWorkspaceProperty("InputBackground", "", direction=Direction.Input),
            doc="Background in counts, with dimensions (y, x, goniometer angle) and a single goniometer angle.",
        )
        self.declareProperty(
            "NormalizedBy",
            "None",
            StringListValidator(["None", "Monitor", "Time"]),
            doc="Flux used to scale the background to each goniometer angle of the sample: the monitor counts or the "
            "duration of each angle. With None, the background is subtracted unscaled.",
        )
        self.declareProperty(
            "NullifyNegativeValues",
            True,
            doc="Set to zero the signal of bins that become negative after the subtraction. Their errors are unchanged.",
        )
        self.declareProperty(
            IMDHistoWorkspaceProperty("OutputWorkspace", "", direction=Direction.Output),
            doc="Sample minus the scaled background. When it is the same workspace as InputWorkspace, the subtraction is done in place.",
        )

    def validateInputs(self) -> Dict[str, str]:
        issues = dict()
        sample = self.getProperty("InputWorkspace").value
        background = self.getProperty("InputBackground").value
        if sample is None or background is None:
            return issues  # the missing mandatory property is reported by its own validator

        for property_name, workspace in (("InputWorkspace", sample), ("InputBackground", background)):
            if workspace.getNumDims() != 3:
                issues[property_name] = f"{property_name} must have 3 dimensions (y, x, goniometer angle)"
            elif workspace.getNumExperimentInfo() == 0:
                issues[property_name] = f"{property_name} must have an instrument"
            elif self._instrument(workspace) is None:
                issues[property_name] = (
                    f"{property_name} has instrument {workspace.getExperimentInfo(0).getInstrumentName()}; "
                    "supported instruments are WAND (HB2C) and DEMAND (HB3A)"
                )
        if issues:
            return issues

        if background.getDimension(2).getNBins() != 1:
            issues["InputBackground"] = (
                f"InputBackground must have exactly one goniometer angle, found {background.getDimension(2).getNBins()}"
            )
            return issues
        sample_pixels = tuple(sample.getDimension(i).getNBins() for i in range(2))
        background_pixels = tuple(background.getDimension(i).getNBins() for i in range(2))
        if sample_pixels != background_pixels:
            issues["InputBackground"] = (
                f"The pixel dimensions of InputBackground {background_pixels} do not match those of InputWorkspace {sample_pixels}"
            )
            return issues
        if self._instrument(sample) != self._instrument(background):
            issues["InputBackground"] = (
                f"InputBackground is from {self._instrument(background)} but InputWorkspace is from {self._instrument(sample)}"
            )
            return issues

        normalized_by = self.getProperty("NormalizedBy").value
        if normalized_by != "None":
            n_angles = sample.getDimension(2).getNBins()
            for property_name, workspace, expected_size in (("InputWorkspace", sample, n_angles), ("InputBackground", background, 1)):
                try:
                    self._flux(workspace, normalized_by, expected_size)
                except ValueError as error:
                    issues[property_name] = f"{property_name}: {error}"
        return issues

    def PyExec(self):
        sample = self.getProperty("InputWorkspace").value
        background = self.getProperty("InputBackground").value
        normalized_by = self.getProperty("NormalizedBy").value
        n_angles = sample.getDimension(2).getNBins()

        # ratio between the flux of each goniometer angle of the sample and the flux of the background
        if normalized_by == "None":
            flux_ratio = np.ones(n_angles)
        else:
            flux_ratio = self._flux(sample, normalized_by, n_angles) / self._flux(background, normalized_by, 1)[0]

        scaled_background = background.getSignalArray() * flux_ratio[np.newaxis, np.newaxis, :]
        scaled_background_error_squared = background.getErrorSquaredArray() * flux_ratio[np.newaxis, np.newaxis, :] ** 2
        signal = sample.getSignalArray() - scaled_background
        error_squared = sample.getErrorSquaredArray() + scaled_background_error_squared
        if self.getProperty("NullifyNegativeValues").value:
            signal[signal < 0] = 0.0

        in_place = self.getPropertyValue("OutputWorkspace") == self.getPropertyValue("InputWorkspace")
        output = sample if in_place else self._clone(sample)
        output.setSignalArray(signal)
        output.setErrorSquaredArray(error_squared)
        self.setProperty("OutputWorkspace", output)

    def _instrument(self, workspace: IMDHistoWorkspace) -> Optional[str]:
        """
        Instrument of a workspace, as a key of the flux-log table.

        :param workspace: MDHistoWorkspace with at least one ExperimentInfo.
        :returns: "WAND", "DEMAND", or None for any other instrument.
        """
        return self._INSTRUMENTS.get(workspace.getExperimentInfo(0).getInstrumentName())

    def _flux(self, workspace: IMDHistoWorkspace, normalized_by: str, expected_size: int) -> np.ndarray:
        """
        Integrated flux of each goniometer angle of a workspace, read from its sample logs.

        :param workspace: MDHistoWorkspace from WAND or DEMAND.
        :param normalized_by: "Monitor" or "Time".
        :param expected_size: number of goniometer angles of the workspace.
        :returns: one flux value per goniometer angle.
        :raises ValueError: if the log is missing, has the wrong size, or holds values that are not finite and positive.
        """
        log_name = self._FLUX_LOGS[self._instrument(workspace)][normalized_by]
        run = workspace.getExperimentInfo(0).run()
        if not run.hasProperty(log_name):
            raise ValueError(f"the '{log_name}' log required by NormalizedBy={normalized_by} is missing")
        flux = np.atleast_1d(np.asarray(run.getProperty(log_name).value, dtype=float))
        if flux.size != expected_size:
            raise ValueError(f"the '{log_name}' log has {flux.size} values, expected one per goniometer angle ({expected_size})")
        if np.any(~np.isfinite(flux)) or np.any(flux <= 0):
            raise ValueError(f"the '{log_name}' log must contain finite, positive values")
        return flux

    def _clone(self, workspace: IMDHistoWorkspace) -> IMDHistoWorkspace:
        clone = self.createChildAlgorithm("CloneWorkspace", enableLogging=False)
        clone.setProperty("InputWorkspace", workspace)
        clone.execute()
        return clone.getProperty("OutputWorkspace").value


AlgorithmFactory.subscribe(HFIRSCDSubtractBackground)
