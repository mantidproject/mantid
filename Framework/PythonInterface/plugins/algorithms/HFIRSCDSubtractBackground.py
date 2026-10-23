# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2026 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from typing import Dict, List, Optional, Tuple

import numpy as np

from mantid.api import (
    AlgorithmFactory,
    IMDHistoWorkspace,
    IMDHistoWorkspaceProperty,
    PropertyMode,
    PythonAlgorithm,
    Workspace,
    WorkspaceGroup,
    WorkspaceProperty,
    mtd,
)
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

    def checkGroups(self) -> bool:
        """
        Disable the automatic processing of WorkspaceGroup inputs, which would run the algorithm once per member.

        A WorkspaceGroup sample is processed by PyExec instead, so that the members share one background, are validated
        together, and are written to one output WorkspaceGroup.

        :returns: False.
        """
        return False

    def PyInit(self):
        self.declareProperty(
            WorkspaceProperty("InputWorkspace", "", direction=Direction.Input),
            doc="Unnormalized sample in detector space, in counts. Either an MDHistoWorkspace with dimensions "
            "(y, x, goniometer angle), as returned by LoadWANDSCD or by HB3AAdjustSampleNorm with OutputType='Detector', "
            "or a WorkspaceGroup of such MDHistoWorkspaces, as returned by HB3AAdjustSampleNorm with OutputType='Detector' "
            "for several scans. Each member of a WorkspaceGroup is processed with its own flux logs.",
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
            WorkspaceProperty("OutputWorkspace", "", direction=Direction.Output),
            doc="Sample minus the scaled background. When it is the same workspace as InputWorkspace, the subtraction is "
            "done in place, member by member for a WorkspaceGroup. Otherwise, for a WorkspaceGroup, a new WorkspaceGroup "
            "with members named <OutputWorkspace>_<input member name>.",
        )
        self.declareProperty(
            WorkspaceProperty("OutputBackgroundWorkspace", "", direction=Direction.Output, optional=PropertyMode.Optional),
            doc="Optional: the background scaled to every goniometer angle of the sample. For a WorkspaceGroup sample, a "
            "WorkspaceGroup with members named <OutputBackgroundWorkspace>_<input member name>.",
        )

    def validateInputs(self) -> Dict[str, str]:
        """
        Check the background first, then every member of the sample against it.

        The background must have 3 dimensions, a supported instrument, a single goniometer angle and, unless
        NormalizedBy is None, a valid flux log. When the background is invalid, the sample is not checked, since the
        comparisons with the background would be meaningless. Each sample member must be an MDHistoWorkspace with 3
        dimensions, a supported instrument, the pixel dimensions and instrument of the background, and a valid flux log.
        The problems of all invalid members are reported together, each prefixed with the member name.

        :returns: dictionary mapping "InputBackground" and/or "InputWorkspace" to a description of the problems found;
            empty when the inputs are valid.
        """
        issues = dict()
        sample = self.getProperty("InputWorkspace").value
        background = self.getProperty("InputBackground").value
        if sample is None or background is None:
            return issues  # the missing mandatory property is reported by its own validator
        normalized_by = self.getProperty("NormalizedBy").value

        background_issue = self._validate_workspace(background)
        if background_issue is None and background.getDimension(2).getNBins() != 1:
            background_issue = f"must have exactly one goniometer angle, found {background.getDimension(2).getNBins()}"
        if background_issue is None and normalized_by != "None":
            background_issue = self._validate_flux(background, normalized_by)
        if background_issue is not None:
            issues["InputBackground"] = f"InputBackground: {background_issue}"
            return issues

        is_group = isinstance(sample, WorkspaceGroup)
        members = self._members(sample)
        messages = ["The input group is empty"] if is_group and not members else []
        for name, member in members:
            label = f"Member '{name}'" if is_group else "InputWorkspace"
            if not isinstance(member, IMDHistoWorkspace):
                messages.append(f"{label}: must be an MDHistoWorkspace")
                continue
            issue = self._validate_workspace(member)
            if issue is None:
                issue = self._validate_against_background(member, background)
            if issue is None and normalized_by != "None":
                issue = self._validate_flux(member, normalized_by)
            if issue is not None:
                messages.append(f"{label}: {issue}")
        if messages:
            issues["InputWorkspace"] = "; ".join(messages)
        return issues

    def PyExec(self):
        """
        Subtract the scaled background from every member of the sample, in place or into new workspaces.

        The subtraction is in place when OutputWorkspace and InputWorkspace have the same name: each member is then
        modified and no workspace of the size of the sample is created. Otherwise each member is cloned first, and a
        WorkspaceGroup sample produces a WorkspaceGroup output whose members are stored in the ADS as
        <OutputWorkspace>_<input member name>. OutputBackgroundWorkspace is created only when a name is given.

        The inputs are assumed valid, as checked by validateInputs.
        """
        sample = self.getProperty("InputWorkspace").value
        background = self.getProperty("InputBackground").value
        normalized_by = self.getProperty("NormalizedBy").value
        nullify = self.getProperty("NullifyNegativeValues").value
        output_name = self.getPropertyValue("OutputWorkspace")
        background_name = self.getPropertyValue("OutputBackgroundWorkspace")
        in_place = output_name == self.getPropertyValue("InputWorkspace")

        outputs, scaled_backgrounds = [], []
        for _, member in self._members(sample):
            flux_ratio = self._flux_ratio(member, background, normalized_by)
            if background_name:
                scaled_backgrounds.append(self._scaled_background(member, background, flux_ratio))
            output = member if in_place else self._clone(member)
            self._subtract(member, background, flux_ratio, nullify, output)
            outputs.append(output)

        if not isinstance(sample, WorkspaceGroup):
            self.setProperty("OutputWorkspace", outputs[0])
            if background_name:
                self.setProperty("OutputBackgroundWorkspace", scaled_backgrounds[0])
            return
        member_names = [name for name, _ in self._members(sample)]
        self.setProperty("OutputWorkspace", sample if in_place else self._group(output_name, member_names, outputs))
        if background_name:
            self.setProperty("OutputBackgroundWorkspace", self._group(background_name, member_names, scaled_backgrounds))

    @staticmethod
    def _members(workspace: Workspace) -> List[Tuple[str, Workspace]]:
        """
        Name and workspace of each member of a WorkspaceGroup, or of a single workspace.

        A single workspace is treated as a group of one member, so that validateInputs and PyExec handle both cases
        with the same loop. The members are not checked; a member may be of any workspace type.

        :param workspace: a WorkspaceGroup, or any single workspace.
        :returns: list of (name, workspace) pairs in the order of the group. A member that is not in the ADS has an
            empty name and is identified by its 1-based position in the group instead. For a single workspace, the
            list has one pair, whose name is empty if the workspace is not in the ADS.
        """
        if not isinstance(workspace, WorkspaceGroup):
            return [(workspace.name(), workspace)]
        members = [workspace.getItem(i) for i in range(workspace.getNumberOfEntries())]
        return [(member.name() or str(i + 1), member) for i, member in enumerate(members)]

    def _instrument(self, workspace: IMDHistoWorkspace) -> Optional[str]:
        """
        Instrument of a workspace, as a key of the flux-log table.

        The instrument name is read from the first ExperimentInfo of the workspace. Both names that a WAND or a DEMAND
        workspace may carry are recognized: WAND or HB2C, and HB3A or DEMAND.

        :param workspace: MDHistoWorkspace with at least one ExperimentInfo.
        :returns: "WAND", "DEMAND", or None for any other instrument.
        :raises RuntimeError: if the workspace has no ExperimentInfo. Callers check getNumExperimentInfo() first.
        """
        return self._INSTRUMENTS.get(workspace.getExperimentInfo(0).getInstrumentName())

    def _validate_workspace(self, workspace: IMDHistoWorkspace) -> Optional[str]:
        """
        Check the dimensions and the instrument of a sample member or of the background.

        The checks are made in order, and each one assumes the previous ones passed: 3 dimensions, at least one
        ExperimentInfo, and an instrument that is WAND or DEMAND.

        :param workspace: a sample member or the background.
        :returns: a description of the first problem found, without the name of the workspace, which the caller adds;
            None when the workspace passes all checks.
        """
        if workspace.getNumDims() != 3:
            return "must have 3 dimensions (y, x, goniometer angle)"
        if workspace.getNumExperimentInfo() == 0:
            return "must have an instrument"
        if self._instrument(workspace) is None:
            return (
                f"has instrument {workspace.getExperimentInfo(0).getInstrumentName()}; "
                "supported instruments are WAND (HB2C) and DEMAND (HB3A)"
            )
        return None

    def _validate_against_background(self, member: IMDHistoWorkspace, background: IMDHistoWorkspace) -> Optional[str]:
        """
        Check that a sample member and the background have the same pixel dimensions and instrument.

        The pixel dimensions are the numbers of bins of the first two dimensions, (y, x). WAND and HB2C, and DEMAND and
        HB3A, count as the same instrument.

        :param member: a sample member that passed _validate_workspace.
        :param background: the background, which passed _validate_workspace.
        :returns: a description of the first problem found, without the name of the member, which the caller adds;
            None when the member matches the background.
        :raises RuntimeError: if either workspace has no ExperimentInfo; this cannot happen after _validate_workspace.
        """
        member_pixels = tuple(member.getDimension(i).getNBins() for i in range(2))
        background_pixels = tuple(background.getDimension(i).getNBins() for i in range(2))
        if member_pixels != background_pixels:
            return f"pixel dimensions {member_pixels} do not match those of InputBackground {background_pixels}"
        if self._instrument(member) != self._instrument(background):
            return f"is from {self._instrument(member)} but InputBackground is from {self._instrument(background)}"
        return None

    def _validate_flux(self, workspace: IMDHistoWorkspace, normalized_by: str) -> Optional[str]:
        """
        Check that the flux log of a workspace has one finite, positive value per goniometer angle.

        This wraps _flux, turning the ValueError it raises for an invalid log into a message for validateInputs.

        :param workspace: a sample member or the background, which passed _validate_workspace.
        :param normalized_by: "Monitor" or "Time".
        :returns: a description of the problem, without the name of the workspace, which the caller adds; None when
            the log is valid.
        """
        try:
            self._flux(workspace, normalized_by)
        except ValueError as error:
            return str(error)
        return None

    def _flux(self, workspace: IMDHistoWorkspace, normalized_by: str) -> np.ndarray:
        """
        Integrated flux of each goniometer angle of a workspace, read from its sample logs.

        The log name depends on the instrument and on normalized_by (see _FLUX_LOGS). The log may be a single value or
        an array, such as the time series written by LoadWANDSCD; either way it must hold one value per goniometer
        angle, the number of bins of the third dimension.

        :param workspace: MDHistoWorkspace from WAND or DEMAND, with at least one ExperimentInfo.
        :param normalized_by: "Monitor" or "Time".
        :returns: one-dimensional array with one flux value per goniometer angle.
        :raises ValueError: if the log is missing, does not hold one value per goniometer angle, or holds values that
            are not finite and positive.
        :raises KeyError: if the instrument is not WAND or DEMAND, or normalized_by is not "Monitor" or "Time"; this
            cannot happen after validateInputs.
        """
        log_name = self._FLUX_LOGS[self._instrument(workspace)][normalized_by]
        run = workspace.getExperimentInfo(0).run()
        if not run.hasProperty(log_name):
            raise ValueError(f"the '{log_name}' log required by NormalizedBy={normalized_by} is missing")
        flux = np.atleast_1d(np.asarray(run.getProperty(log_name).value, dtype=float))
        n_angles = workspace.getDimension(2).getNBins()
        if flux.size != n_angles:
            raise ValueError(f"the '{log_name}' log has {flux.size} values, expected one per goniometer angle ({n_angles})")
        if np.any(~np.isfinite(flux)) or np.any(flux <= 0):
            raise ValueError(f"the '{log_name}' log must contain finite, positive values")
        return flux

    def _flux_ratio(self, member: IMDHistoWorkspace, background: IMDHistoWorkspace, normalized_by: str) -> np.ndarray:
        """
        Ratio r(θ) = F_S(θ) / F_B between the flux of each goniometer angle of a sample member and the flux of the
        background, the factor applied to the background before it is subtracted from angle θ of the member.

        :param member: a sample member.
        :param background: the background, with a single goniometer angle.
        :param normalized_by: "None", "Monitor" or "Time".
        :returns: one-dimensional array with one ratio per goniometer angle of the member; all ones when normalized_by
            is "None", in which case the flux logs are not read.
        :raises ValueError: if a flux log is invalid (see _flux); this cannot happen after validateInputs.
        """
        if normalized_by == "None":
            return np.ones(member.getDimension(2).getNBins())
        return self._flux(member, normalized_by) / self._flux(background, normalized_by)[0]

    @staticmethod
    def _subtract(
        member: IMDHistoWorkspace, background: IMDHistoWorkspace, flux_ratio: np.ndarray, nullify: bool, output: IMDHistoWorkspace
    ) -> None:
        """
        Write member - flux_ratio * background into output, which may be the member itself.

        For each goniometer angle θ, the signal is S(θ) - r(θ) B and the error squared is σ_S²(θ) + r(θ)² σ_B². With
        nullify, negative signals are then set to zero; their errors keep the propagated value.

        The background is subtracted one goniometer angle at a time, so that the only array as large as the member is
        the copy of its signal (then of its errors) that is written back into output. The signal and the errors of the
        member are read before output is written, so output may be the member itself.

        :param member: a sample member, with dimensions (y, x, goniometer angle).
        :param background: the background, with the pixel dimensions of the member and a single goniometer angle.
        :param flux_ratio: one ratio per goniometer angle of the member (see _flux_ratio).
        :param nullify: whether to set negative signals to zero.
        :param output: workspace with the dimensions of the member that receives the result: the member itself, or a
            clone of it.
        :raises ValueError: if the dimensions of output do not match those of the member; this cannot happen for the
            member itself or a clone of it.
        """
        background_signal = background.getSignalArray()[:, :, 0]
        signal = np.array(member.getSignalArray())
        for angle, ratio in enumerate(flux_ratio):
            signal[:, :, angle] -= ratio * background_signal
        if nullify:
            np.maximum(signal, 0.0, out=signal)
        output.setSignalArray(signal)
        del signal

        background_error_squared = background.getErrorSquaredArray()[:, :, 0]
        error_squared = np.array(member.getErrorSquaredArray())
        for angle, ratio in enumerate(flux_ratio):
            error_squared[:, :, angle] += ratio**2 * background_error_squared
        output.setErrorSquaredArray(error_squared)

    def _scaled_background(self, member: IMDHistoWorkspace, background: IMDHistoWorkspace, flux_ratio: np.ndarray) -> IMDHistoWorkspace:
        """
        Background scaled to every goniometer angle of a sample member, for OutputBackgroundWorkspace.

        The result is a clone of the member, so it has the dimensions, goniometer settings and sample logs of the
        member, with signal r(θ) B and error squared r(θ)² σ_B² at each goniometer angle θ. It is not added to the ADS.

        :param member: a sample member, with dimensions (y, x, goniometer angle).
        :param background: the background, with the pixel dimensions of the member and a single goniometer angle.
        :param flux_ratio: one ratio per goniometer angle of the member (see _flux_ratio).
        :returns: new MDHistoWorkspace of the size of the member.
        :raises RuntimeError: if the member cannot be cloned (see _clone).
        """
        scaled = self._clone(member)
        shape = member.getSignalArray().shape
        signal = np.empty(shape)
        error_squared = np.empty(shape)
        for angle, ratio in enumerate(flux_ratio):
            signal[:, :, angle] = ratio * background.getSignalArray()[:, :, 0]
            error_squared[:, :, angle] = ratio**2 * background.getErrorSquaredArray()[:, :, 0]
        scaled.setSignalArray(signal)
        scaled.setErrorSquaredArray(error_squared)
        return scaled

    @staticmethod
    def _group(name: str, member_names: List[str], workspaces: List[Workspace]) -> WorkspaceGroup:
        """
        Group workspaces under name, each one stored in the ADS as <name>_<member name>, as in HB3AAdjustSampleNorm.

        The group itself is not added to the ADS; setting it as an output property does that, under name. An existing
        workspace in the ADS with the name of a new member is replaced.

        :param name: name of the output property value, OutputWorkspace or OutputBackgroundWorkspace.
        :param member_names: names of the input members, in the order of workspaces.
        :param workspaces: workspaces to group, one per input member.
        :returns: new WorkspaceGroup holding the workspaces in the given order.
        """
        group = WorkspaceGroup()
        for member_name, workspace in zip(member_names, workspaces):
            mtd.addOrReplace(f"{name}_{member_name}", workspace)
            group.addWorkspace(workspace)
        return group

    def _clone(self, workspace: IMDHistoWorkspace) -> IMDHistoWorkspace:
        """
        Copy of a workspace, made with a child CloneWorkspace so that the copy is not added to the ADS.

        :param workspace: the MDHistoWorkspace to copy, with its ExperimentInfo.
        :returns: new MDHistoWorkspace with the dimensions, data and ExperimentInfo of the workspace.
        :raises RuntimeError: if CloneWorkspace fails, for instance when there is not enough memory.
        """
        clone = self.createChildAlgorithm("CloneWorkspace", enableLogging=False)
        clone.setProperty("InputWorkspace", workspace)
        clone.execute()
        return clone.getProperty("OutputWorkspace").value


AlgorithmFactory.subscribe(HFIRSCDSubtractBackground)
