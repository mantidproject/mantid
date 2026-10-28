# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2026 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from typing import Dict, List, Optional, Tuple, Union

import numpy as np

from mantid.api import (
    AlgorithmFactory,
    ExperimentInfo,
    IMDHistoWorkspace,
    MatrixWorkspace,
    PropertyMode,
    PythonAlgorithm,
    Workspace,
    WorkspaceGroup,
    WorkspaceProperty,
    mtd,
)
from mantid.dataobjects import GroupingWorkspace
from mantid.kernel import Direction, StringListValidator

# A background is either an MDHistoWorkspace with a single goniometer angle, or a Workspace2D with one value per spectrum
Background = Union[IMDHistoWorkspace, MatrixWorkspace]


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
            WorkspaceProperty("InputBackground", "", direction=Direction.Input),
            doc="Background in counts, from the instrument of the sample: either an MDHistoWorkspace with dimensions "
            "(y, x, goniometer angle) and a single goniometer angle, or a Workspace2D with one value per spectrum, "
            "whose spectra are matched to the pixels of the sample through their detector IDs.",
        )
        self.declareProperty(
            WorkspaceProperty("InputGroupingWorkspace", "", direction=Direction.Input, optional=PropertyMode.Optional),
            doc="Optional: GroupingWorkspace for a sample whose pixels were grouped while the background's were not, "
            "such as the OutputGroupingWorkspace of LoadWANDSCD or HB3AAdjustSampleNorm. The intensities of the "
            "background are summed within each group before the subtraction. To be used only with an ungrouped "
            "background: a grouped background is rejected when InputGroupingWorkspace is provided.",
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

        InputGroupingWorkspace, when given, must be a GroupingWorkspace. The background must have a supported instrument,
        a single goniometer angle (MDHistoWorkspace) or a single value per spectrum (Workspace2D) and, unless NormalizedBy
        is None, a valid flux log; with InputGroupingWorkspace, it must also be ungrouped (see _validate_background).
        When the grouping workspace or the background is invalid, the sample is not checked, since the comparisons with
        them would be meaningless. Each sample member must be an MDHistoWorkspace with 3 dimensions, a supported
        instrument, pixels that match the background (see _validate_against_background), and a valid flux log. The
        problems of all invalid members are reported together, each prefixed with the member name.

        The detector IDs themselves are looked up only in PyExec, which raises before modifying any member if one is
        missing or does not have a spectrum of its own.

        :returns: dictionary mapping "InputGroupingWorkspace", "InputBackground" and/or "InputWorkspace" to a
            description of the problems found; empty when the inputs are valid.
        """
        issues = dict()
        sample = self.getProperty("InputWorkspace").value
        background = self.getProperty("InputBackground").value
        if sample is None or background is None:
            return issues  # the missing mandatory property is reported by its own validator
        normalized_by = self.getProperty("NormalizedBy").value

        grouping = self.getProperty("InputGroupingWorkspace").value
        n_groups = None
        if grouping is not None:
            if not isinstance(grouping, GroupingWorkspace):
                issues["InputGroupingWorkspace"] = "InputGroupingWorkspace: must be a GroupingWorkspace"
                return issues
            group_ids = grouping.extractY()
            n_groups = np.unique(group_ids[group_ids > 0]).size

        background_issue = self._validate_background(background, grouping)
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
                issue = self._validate_against_background(member, background, grouping, n_groups)
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

        The background is first arranged in the pixel layout of every member (see _arranged_background), before any
        member is subtracted; members with the same pixel layout share this arrangement.

        The inputs are assumed valid, as checked by validateInputs.

        :raises RuntimeError: if a detector ID of the sample or of the background is not found in InputBackground or in
            InputGroupingWorkspace, or does not have a spectrum of its own there (see _workspace_indices). No member is
            modified then.
        """
        sample = self.getProperty("InputWorkspace").value
        background = self.getProperty("InputBackground").value
        grouping = self.getProperty("InputGroupingWorkspace").value
        normalized_by = self.getProperty("NormalizedBy").value
        nullify = self.getProperty("NullifyNegativeValues").value
        output_name = self.getPropertyValue("OutputWorkspace")
        background_name = self.getPropertyValue("OutputBackgroundWorkspace")
        in_place = output_name == self.getPropertyValue("InputWorkspace")

        # Arrange the background for every member before the first subtraction, so that a detector ID that is not found
        # raises before any member is modified in place
        arrangements = dict()  # background arranged in the pixel layout of the members, by detectorID log or None
        arranged = [
            (
                member,
                self._flux_ratio(member, background, normalized_by),
                self._arranged_background(member, background, grouping, arrangements),
            )
            for _, member in self._members(sample)
        ]
        outputs, scaled_backgrounds = [], []
        for member, flux_ratio, background_arrays in arranged:
            if background_name:
                scaled_backgrounds.append(self._scaled_background(member, *background_arrays, flux_ratio))
            output = member if in_place else self._clone(member)
            self._subtract(member, *background_arrays, flux_ratio, nullify, output)
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

    @staticmethod
    def _experiment_info(workspace: Background) -> ExperimentInfo:
        """
        ExperimentInfo holding the instrument and the sample logs of a workspace.

        :param workspace: an MDHistoWorkspace, whose first ExperimentInfo is returned, or a MatrixWorkspace, which is
            itself an ExperimentInfo.
        :returns: the ExperimentInfo of the workspace.
        :raises RuntimeError: if an MDHistoWorkspace has no ExperimentInfo. Callers check getNumExperimentInfo() first.
        """
        if isinstance(workspace, MatrixWorkspace):
            return workspace
        return workspace.getExperimentInfo(0)

    @staticmethod
    def _n_angles(workspace: Background) -> int:
        """
        Number of goniometer angles of a workspace: the number of bins of the third dimension of an MDHistoWorkspace,
        and 1 for a Workspace2D background.
        """
        if isinstance(workspace, MatrixWorkspace):
            return 1
        return workspace.getDimension(2).getNBins()

    def _instrument(self, workspace: Background) -> Optional[str]:
        """
        Instrument of a workspace, as a key of the flux-log table.

        The instrument name is read from the ExperimentInfo of the workspace (see _experiment_info). Both names that a
        WAND or a DEMAND workspace may carry are recognized: WAND or HB2C, and HB3A or DEMAND.

        :param workspace: MDHistoWorkspace with at least one ExperimentInfo, or MatrixWorkspace.
        :returns: "WAND", "DEMAND", or None for any other instrument.
        :raises RuntimeError: if an MDHistoWorkspace has no ExperimentInfo. Callers check getNumExperimentInfo() first.
        """
        return self._INSTRUMENTS.get(self._experiment_info(workspace).getInstrumentName())

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

    def _validate_background(self, background: Workspace, grouping: Optional[GroupingWorkspace]) -> Optional[str]:
        """
        Check the type, shape, instrument and, with a grouping workspace, the pixels of the background.

        An MDHistoWorkspace background must pass _validate_workspace and have a single goniometer angle. A Workspace2D
        background must have one value per spectrum and a supported instrument. With a grouping workspace, the
        background must not be grouped, in any way: it must have one pixel or spectrum per spectrum of the grouping
        workspace, which has one spectrum per detector. A background grouped like the sample, more finely, or more
        coarsely, has fewer pixels and is rejected. An MDHistoWorkspace background must also have a 'detectorID' log
        identifying the detector of each pixel.

        :param background: the InputBackground property value, of any workspace type.
        :param grouping: the InputGroupingWorkspace, or None.
        :returns: a description of the first problem found, without the name of the property, which the caller adds;
            None when the background is valid.
        """
        if isinstance(background, MatrixWorkspace):
            if background.blocksize() != 1:
                return f"a Workspace2D background must have one value per spectrum, found {background.blocksize()}"
            if self._instrument(background) is None:
                return f"has instrument {background.getInstrumentName()}; supported instruments are WAND (HB2C) and DEMAND (HB3A)"
            n_elements = background.getNumberHistograms()
        elif isinstance(background, IMDHistoWorkspace):
            issue = self._validate_workspace(background)
            if issue is not None:
                return issue
            if background.getDimension(2).getNBins() != 1:
                return f"must have exactly one goniometer angle, found {background.getDimension(2).getNBins()}"
            n_elements = self._n_pixels(background)
        else:
            return "must be an MDHistoWorkspace or a Workspace2D"
        if grouping is None:
            return None
        if n_elements != grouping.getNumberHistograms():
            n_detectors = grouping.getNumberHistograms()
            if isinstance(background, MatrixWorkspace):
                return f"must have one spectrum per spectrum of InputGroupingWorkspace ({n_detectors}), found {n_elements}"
            return (
                "must not be grouped when InputGroupingWorkspace is provided: it has "
                f"{n_elements} pixels, but an ungrouped background has one pixel per spectrum of InputGroupingWorkspace "
                f"({n_detectors}). If InputBackground was loaded with the same grouping as InputWorkspace, do not "
                "provide InputGroupingWorkspace"
            )
        if isinstance(background, IMDHistoWorkspace):
            return self._validate_detector_ids(background, "required with InputGroupingWorkspace")
        return None

    @staticmethod
    def _n_pixels(workspace: IMDHistoWorkspace) -> int:
        """
        Number of pixels of an MDHistoWorkspace with dimensions (y, x, goniometer angle): the product of the numbers of
        bins of its first two dimensions.
        """
        return workspace.getDimension(0).getNBins() * workspace.getDimension(1).getNBins()

    def _validate_detector_ids(self, workspace: IMDHistoWorkspace, reason: str) -> Optional[str]:
        """
        Check that an MDHistoWorkspace has a 'detectorID' log with one detector ID per pixel.

        LoadWANDSCD and HB3AAdjustSampleNorm write this log; its entry k is the detector ID, or the first detector ID of
        the group, of pixel k, the pixels being ordered with the first dimension varying fastest.

        :param workspace: a sample member or an MDHistoWorkspace background, which passed _validate_workspace.
        :param reason: why the log is required, appended to the message when it is missing.
        :returns: a description of the problem, without the name of the workspace, which the caller adds; None when
            the log is valid.
        """
        run = workspace.getExperimentInfo(0).run()
        if not run.hasProperty("detectorID"):
            return f"the 'detectorID' log, {reason}, is missing"
        n_ids = np.asarray(run.getProperty("detectorID").value).size
        if n_ids != self._n_pixels(workspace):
            return f"the 'detectorID' log has {n_ids} values, expected one per pixel ({self._n_pixels(workspace)})"
        return None

    def _validate_against_background(
        self, member: IMDHistoWorkspace, background: Background, grouping: Optional[GroupingWorkspace], n_groups: Optional[int]
    ) -> Optional[str]:
        """
        Check that a sample member and the background have the same instrument and matching pixels.

        WAND and HB2C, and DEMAND and HB3A, count as the same instrument. How the pixels must match depends on the
        inputs:

        - With a grouping workspace, the member must have one pixel per group, and a 'detectorID' log to find the
          group of each pixel.
        - With a Workspace2D background and no grouping workspace, the member must have one pixel per spectrum of the
          background, and a 'detectorID' log to find the spectrum of each pixel.
        - With an MDHistoWorkspace background and no grouping workspace, the member and the background must have the
          same pixel dimensions, the numbers of bins of their first two dimensions (y, x).

        :param member: a sample member that passed _validate_workspace.
        :param background: the background, which passed _validate_background.
        :param grouping: the InputGroupingWorkspace, or None.
        :param n_groups: number of groups of the grouping workspace, or None without one.
        :returns: a description of the first problem found, without the name of the member, which the caller adds;
            None when the member matches the background.
        :raises RuntimeError: if a workspace has no ExperimentInfo; this cannot happen after _validate_workspace.
        """
        if self._instrument(member) != self._instrument(background):
            return f"is from {self._instrument(member)} but InputBackground is from {self._instrument(background)}"
        n_pixels = self._n_pixels(member)
        if grouping is not None:
            if n_pixels != n_groups:
                return f"has {n_pixels} pixels but InputGroupingWorkspace has {n_groups} groups"
            return self._validate_detector_ids(member, "required with InputGroupingWorkspace")
        if isinstance(background, MatrixWorkspace):
            if n_pixels != background.getNumberHistograms():
                return (
                    f"has {n_pixels} pixels but InputBackground has {background.getNumberHistograms()} spectra; "
                    "if InputWorkspace was grouped, provide InputGroupingWorkspace"
                )
            return self._validate_detector_ids(member, "required with a Workspace2D background")
        member_pixels = tuple(member.getDimension(i).getNBins() for i in range(2))
        background_pixels = tuple(background.getDimension(i).getNBins() for i in range(2))
        if member_pixels != background_pixels:
            return (
                f"pixel dimensions {member_pixels} do not match those of InputBackground {background_pixels}; "
                "if InputWorkspace was grouped and InputBackground was not, provide InputGroupingWorkspace"
            )
        return None

    def _validate_flux(self, workspace: IMDHistoWorkspace, normalized_by: str) -> Optional[str]:
        """
        Check that the flux log of a workspace has one finite, positive value per goniometer angle.

        This wraps _flux, turning the ValueError it raises for an invalid log into a message for validateInputs.

        :param workspace: a sample member that passed _validate_workspace, or the background, which passed
            _validate_background.
        :param normalized_by: "Monitor" or "Time".
        :returns: a description of the problem, without the name of the workspace, which the caller adds; None when
            the log is valid.
        """
        try:
            self._flux(workspace, normalized_by)
        except ValueError as error:
            return str(error)
        return None

    def _flux(self, workspace: Background, normalized_by: str) -> np.ndarray:
        """
        Integrated flux of each goniometer angle of a workspace, read from its sample logs.

        The log name depends on the instrument and on normalized_by (see _FLUX_LOGS). The log may be a single value or
        an array, such as the time series written by LoadWANDSCD; either way it must hold one value per goniometer
        angle: the number of bins of the third dimension of an MDHistoWorkspace, and a single value for a Workspace2D
        background.

        :param workspace: MDHistoWorkspace from WAND or DEMAND, with at least one ExperimentInfo, or Workspace2D
            background from WAND or DEMAND.
        :param normalized_by: "Monitor" or "Time".
        :returns: one-dimensional array with one flux value per goniometer angle.
        :raises ValueError: if the log is missing, does not hold one value per goniometer angle, or holds values that
            are not finite and positive.
        :raises KeyError: if the instrument is not WAND or DEMAND, or normalized_by is not "Monitor" or "Time"; this
            cannot happen after validateInputs.
        """
        log_name = self._FLUX_LOGS[self._instrument(workspace)][normalized_by]
        run = self._experiment_info(workspace).run()
        if not run.hasProperty(log_name):
            raise ValueError(f"the '{log_name}' log required by NormalizedBy={normalized_by} is missing")
        flux = np.atleast_1d(np.asarray(run.getProperty(log_name).value, dtype=float))
        n_angles = self._n_angles(workspace)
        if flux.size != n_angles:
            raise ValueError(f"the '{log_name}' log has {flux.size} values, expected one per goniometer angle ({n_angles})")
        if np.any(~np.isfinite(flux)) or np.any(flux <= 0):
            raise ValueError(f"the '{log_name}' log must contain finite, positive values")
        return flux

    def _flux_ratio(self, member: IMDHistoWorkspace, background: Background, normalized_by: str) -> np.ndarray:
        """
        Ratio r(θ) = F_S(θ) / F_B between the flux of each goniometer angle of a sample member and the flux of the
        background, the factor applied to the background before it is subtracted from angle θ of the member.

        :param member: a sample member.
        :param background: the background, with a single goniometer angle or a single value per spectrum.
        :param normalized_by: "None", "Monitor" or "Time".
        :returns: one-dimensional array with one ratio per goniometer angle of the member; all ones when normalized_by
            is "None", in which case the flux logs are not read.
        :raises ValueError: if a flux log is invalid (see _flux); this cannot happen after validateInputs.
        """
        if normalized_by == "None":
            return np.ones(member.getDimension(2).getNBins())
        return self._flux(member, normalized_by) / self._flux(background, normalized_by)[0]

    def _arranged_background(
        self,
        member: IMDHistoWorkspace,
        background: Background,
        grouping: Optional[GroupingWorkspace],
        arrangements: Dict[Optional[bytes], Tuple[np.ndarray, np.ndarray]],
    ) -> Tuple[np.ndarray, np.ndarray]:
        """
        Signal and error squared of the background, arranged in the pixel layout of a sample member.

        - An MDHistoWorkspace background without a grouping workspace already has the pixel layout of the member. Its
          signal and errors are copied, so that a subtraction in place does not change them when the background is
          also a member of the sample.
        - A Workspace2D background without a grouping workspace: pixel k of the member takes the value of the spectrum
          whose detector ID is entry k of the 'detectorID' log of the member.
        - With a grouping workspace: the intensities and errors squared of the background are summed within each
          group, the group of each background pixel or spectrum being that of its detector ID. Pixel k of the member
          then takes the sums of the group of entry k of its 'detectorID' log, the first detector ID of the group.

        The arrangement depends only on the 'detectorID' log of the member, so it is computed once per distinct log and
        stored in arrangements. The copy of an MDHistoWorkspace background without a grouping workspace is the same
        for all members, and is stored under the key None.

        :param member: a sample member that passed validateInputs.
        :param background: the background, which passed validateInputs.
        :param grouping: the InputGroupingWorkspace, or None.
        :param arrangements: cache of the arrangements already computed, by 'detectorID' log, or None for the copy of
            an MDHistoWorkspace background without a grouping workspace; updated in place.
        :returns: the signal and the error squared of the background, two arrays with the shape of the first two
            dimensions of the member.
        :raises RuntimeError: if a detector ID is not found in the background or in the grouping workspace, or does not
            have a spectrum of its own there (see _workspace_indices).
        """
        if grouping is None and isinstance(background, IMDHistoWorkspace):
            if None not in arrangements:
                arrangements[None] = (np.array(background.getSignalArray()[:, :, 0]), np.array(background.getErrorSquaredArray()[:, :, 0]))
            return arrangements[None]
        pixel_ids = np.asarray(member.getExperimentInfo(0).run().getProperty("detectorID").value, dtype=int)
        key = pixel_ids.tobytes()
        if key not in arrangements:
            if grouping is None:
                indices = self._workspace_indices(
                    background, pixel_ids, "InputBackground", "; a grouped InputBackground must be grouped like InputWorkspace"
                )
                signal = background.extractY()[indices, 0]
                error_squared = background.extractE()[indices, 0] ** 2
            else:
                element_signal, element_error_squared, element_ids = self._background_elements(background)
                group_ids = grouping.extractY()[:, 0].astype(int)
                element_groups = group_ids[self._workspace_indices(grouping, element_ids, "InputGroupingWorkspace")]
                pixel_groups = group_ids[self._workspace_indices(grouping, pixel_ids, "InputGroupingWorkspace")]
                n_bins = group_ids.max() + 1
                signal = np.bincount(element_groups, weights=element_signal, minlength=n_bins)[pixel_groups]
                error_squared = np.bincount(element_groups, weights=element_error_squared, minlength=n_bins)[pixel_groups]
            shape = member.getSignalArray().shape[:2]
            arrangements[key] = (signal.reshape(shape, order="F"), error_squared.reshape(shape, order="F"))
        return arrangements[key]

    def _background_elements(self, background: Background) -> Tuple[np.ndarray, np.ndarray, np.ndarray]:
        """
        Signal, error squared and detector ID of every pixel or spectrum of an ungrouped background.

        :param background: an MDHistoWorkspace background with a 'detectorID' log, whose pixels are ordered with the
            first dimension varying fastest, or a Workspace2D background with one detector per spectrum.
        :returns: three one-dimensional arrays with one entry per pixel or spectrum.
        :raises RuntimeError: if a spectrum of a Workspace2D background does not have exactly one detector.
        """
        if isinstance(background, IMDHistoWorkspace):
            signal = background.getSignalArray()[:, :, 0].ravel(order="F")
            error_squared = background.getErrorSquaredArray()[:, :, 0].ravel(order="F")
            ids = np.asarray(background.getExperimentInfo(0).run().getProperty("detectorID").value, dtype=int)
            return signal, error_squared, ids
        # detector ID of each spectrum, found by looking up the workspace index of every detector of the instrument
        detector_ids = np.asarray(background.detectorInfo().detectorIDs(), dtype=int)
        n_spectra = background.getNumberHistograms()
        if detector_ids.size != n_spectra:
            raise RuntimeError(
                f"InputBackground has {n_spectra} spectra for {detector_ids.size} detectors; each spectrum must have exactly one detector"
            )
        # with as many detectors as spectra, and each detector in a spectrum of its own, every spectrum gets one ID
        ids = np.empty(n_spectra, dtype=int)
        ids[self._workspace_indices(background, detector_ids, "InputBackground", "; each spectrum must have exactly one detector")] = (
            detector_ids
        )
        return background.extractY()[:, 0], background.extractE()[:, 0] ** 2, ids

    @staticmethod
    def _workspace_indices(workspace: MatrixWorkspace, detector_ids: np.ndarray, name: str, hint: str = "") -> np.ndarray:
        """
        Workspace index of the spectrum holding each detector ID, each detector ID having a spectrum of its own.

        The detector IDs looked up are those of distinct pixels, groups or detectors, so two of them in the same
        spectrum mean that the workspace is grouped differently from what the caller expects.

        :param workspace: a Workspace2D background or a GroupingWorkspace, whose spectra have detector IDs.
        :param detector_ids: one-dimensional array of distinct detector IDs.
        :param name: property name of the workspace, for the error message.
        :param hint: what the workspace must be, appended to the error message when detector IDs share a spectrum.
        :returns: one-dimensional array of distinct workspace indices, in the order of detector_ids.
        :raises RuntimeError: if a detector ID is not found in the workspace, is in several spectra, or shares a
            spectrum with another detector ID.
        """
        indices = np.asarray(workspace.getIndicesFromDetectorIDs(detector_ids.tolist()), dtype=int)
        if indices.size < detector_ids.size:
            raise RuntimeError(
                f"{detector_ids.size - indices.size} of {detector_ids.size} detector IDs are not found in {name}; "
                "the sample, the background and the grouping workspace must come from the same instrument"
            )
        if indices.size > detector_ids.size:
            raise RuntimeError(f"{indices.size - detector_ids.size} detector IDs are in several spectra of {name}")
        n_shared = indices.size - np.unique(indices).size
        if n_shared > 0:
            raise RuntimeError(f"{n_shared} of {detector_ids.size} detector IDs share a spectrum of {name} with another one{hint}")
        return indices

    @staticmethod
    def _subtract(
        member: IMDHistoWorkspace,
        background_signal: np.ndarray,
        background_error_squared: np.ndarray,
        flux_ratio: np.ndarray,
        nullify: bool,
        output: IMDHistoWorkspace,
    ) -> None:
        """
        Write member - flux_ratio * background into output, which may be the member itself.

        For each goniometer angle θ, the signal is S(θ) - r(θ) B and the error squared is σ_S²(θ) + r(θ)² σ_B². With
        nullify, negative signals are then set to zero; their errors keep the propagated value.

        The background is subtracted one goniometer angle at a time, so that the only array as large as the member is
        the copy of its signal (then of its errors) that is written back into output. The signal and the errors of the
        member are read before output is written, so output may be the member itself.

        :param member: a sample member, with dimensions (y, x, goniometer angle).
        :param background_signal: signal of the background in the pixel layout of the member (see
            _arranged_background), an array with the shape of the first two dimensions of the member.
        :param background_error_squared: error squared of the background, with the same shape.
        :param flux_ratio: one ratio per goniometer angle of the member (see _flux_ratio).
        :param nullify: whether to set negative signals to zero.
        :param output: workspace with the dimensions of the member that receives the result: the member itself, or a
            clone of it.
        :raises ValueError: if the dimensions of output do not match those of the member; this cannot happen for the
            member itself or a clone of it.
        """
        signal = np.array(member.getSignalArray())
        for angle, ratio in enumerate(flux_ratio):
            signal[:, :, angle] -= ratio * background_signal
        if nullify:
            np.maximum(signal, 0.0, out=signal)
        output.setSignalArray(signal)
        del signal

        error_squared = np.array(member.getErrorSquaredArray())
        for angle, ratio in enumerate(flux_ratio):
            error_squared[:, :, angle] += ratio**2 * background_error_squared
        output.setErrorSquaredArray(error_squared)

    def _scaled_background(
        self, member: IMDHistoWorkspace, background_signal: np.ndarray, background_error_squared: np.ndarray, flux_ratio: np.ndarray
    ) -> IMDHistoWorkspace:
        """
        Background scaled to every goniometer angle of a sample member, for OutputBackgroundWorkspace.

        The result is a clone of the member, so it has the dimensions, goniometer settings and sample logs of the
        member, with signal r(θ) B and error squared r(θ)² σ_B² at each goniometer angle θ. It is not added to the ADS.

        :param member: a sample member, with dimensions (y, x, goniometer angle).
        :param background_signal: signal of the background in the pixel layout of the member (see
            _arranged_background), an array with the shape of the first two dimensions of the member.
        :param background_error_squared: error squared of the background, with the same shape.
        :param flux_ratio: one ratio per goniometer angle of the member (see _flux_ratio).
        :returns: new MDHistoWorkspace of the size of the member.
        :raises RuntimeError: if the member cannot be cloned (see _clone).
        """
        scaled = self._clone(member)
        shape = member.getSignalArray().shape
        signal = np.empty(shape)
        error_squared = np.empty(shape)
        for angle, ratio in enumerate(flux_ratio):
            signal[:, :, angle] = ratio * background_signal
            error_squared[:, :, angle] = ratio**2 * background_error_squared
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
