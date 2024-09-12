# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
# pylint: disable=invalid-name

"""SANSSingleReduction version 2 algorithm performs a single reduction on event sliced data."""

from copy import deepcopy

from SANSSingleReductionBase import SANSSingleReductionBase

from mantid.api import AlgorithmFactory, AnalysisDataService, MatrixWorkspaceProperty, PropertyMode, WorkspaceGroup, WorkspaceGroupProperty
from mantid.kernel import Direction
from mantid.simpleapi import CloneWorkspace
from sans.algorithm_detail.bundles import EventSliceSettingBundle
from sans.algorithm_detail.single_execution import (
    run_initial_event_slice_reduction,
    run_core_event_slice_reduction,
    get_reduction_mode_vs_output_bundles,
    run_optimized_for_can,
)
from sans.common.enums import DataType, ReductionMode, FitType
from sans.common.general_functions import (
    create_child_algorithm,
    does_can_workspace_exist_on_ads,
    get_transmission_output_name,
    get_output_name,
)


class SANSSingleReduction(SANSSingleReductionBase):
    def category(self):
        return "SANS\\Reduction"

    def version(self):
        return 2

    def summary(self):
        return "Performs a single reduction of SANS data, optimised for event slices."

    def _declare_output_properties(self):
        self.declareProperty(
            MatrixWorkspaceProperty("OutShiftAndScaleFactor", "", optional=PropertyMode.Optional, direction=Direction.Output),
            doc="A workspace containing the applied shift factor as X data and applied scale factor " "as Y data.",
        )

        # This breaks our flexibility with the reduction mode. We need to check if we can populate this based on
        # the available reduction modes for the state input. TODO: check if this is possible
        self.declareProperty(
            WorkspaceGroupProperty("OutputWorkspaceLAB", "", optional=PropertyMode.Optional, direction=Direction.Output),
            doc="The output workspace for the low-angle bank.",
        )
        self.declareProperty(
            WorkspaceGroupProperty("OutputWorkspaceHAB", "", optional=PropertyMode.Optional, direction=Direction.Output),
            doc="The output workspace for the high-angle bank.",
        )
        self.declareProperty(
            WorkspaceGroupProperty("OutputWorkspaceMerged", "", optional=PropertyMode.Optional, direction=Direction.Output),
            doc="The output workspace for the merged reduction.",
        )
        self.setPropertyGroup("OutShiftAndScaleFactor", "Output")
        self.setPropertyGroup("OutputWorkspaceLAB", "Output")
        self.setPropertyGroup("OutputWorkspaceHAB", "Output")
        self.setPropertyGroup("OutputWorkspaceMerged", "Output")

        # CAN output
        self.declareProperty(
            WorkspaceGroupProperty("OutputWorkspaceLABCan", "", optional=PropertyMode.Optional, direction=Direction.Output),
            doc="The can output workspace group for the low-angle bank, provided there is one. "
            "Each workspace in the group is one event slice.",
        )
        self.declareProperty(
            WorkspaceGroupProperty("OutputWorkspaceHABCan", "", optional=PropertyMode.Optional, direction=Direction.Output),
            doc="The can output workspace group for the high-angle bank, provided there is one. "
            "Each workspace in the group is one event slice.",
        )
        self.declareProperty(
            WorkspaceGroupProperty("OutputWorkspaceLABSample", "", optional=PropertyMode.Optional, direction=Direction.Output),
            doc="The sample output workspace group for the low-angle bank, provided there is one. "
            "Each workspace in the group is one event slice.",
        )
        self.declareProperty(
            WorkspaceGroupProperty("OutputWorkspaceHABSample", "", optional=PropertyMode.Optional, direction=Direction.Output),
            doc="The sample output workspace group for the high-angle bank, provided there is one. "
            "Each workspace in the group is one event slice.",
        )
        self.declareProperty(
            MatrixWorkspaceProperty(
                "OutputWorkspaceCalculatedTransmission", "", optional=PropertyMode.Optional, direction=Direction.Output
            ),
            doc="The calculated transmission workspace.",
        )
        self.declareProperty(
            MatrixWorkspaceProperty("OutputWorkspaceUnfittedTransmission", "", optional=PropertyMode.Optional, direction=Direction.Output),
            doc="The unfitted transmission workspace.",
        )
        self.declareProperty(
            MatrixWorkspaceProperty(
                "OutputWorkspaceCalculatedTransmissionCan", "", optional=PropertyMode.Optional, direction=Direction.Output
            ),
            doc="The calculated transmission workspace for the can.",
        )
        self.declareProperty(
            MatrixWorkspaceProperty(
                "OutputWorkspaceUnfittedTransmissionCan", "", optional=PropertyMode.Optional, direction=Direction.Output
            ),
            doc="The unfitted transmission workspace for the can.",
        )
        self.setPropertyGroup("OutputWorkspaceLABCan", "Can Output")
        self.setPropertyGroup("OutputWorkspaceHABCan", "Can Output")
        self.setPropertyGroup("OutputWorkspaceLABSample", "Can Output")
        self.setPropertyGroup("OutputWorkspaceHABSample", "Can Output")

        # Output CAN Count and Norm for optimizations
        self.declareProperty(
            WorkspaceGroupProperty("OutputWorkspaceLABCanNorm", "", optional=PropertyMode.Optional, direction=Direction.Output),
            doc="The can norm output workspace group for the low-angle bank, provided there is one. "
            "Each workspace in the group is one event slice.",
        )
        self.declareProperty(
            WorkspaceGroupProperty("OutputWorkspaceLABCanCount", "", optional=PropertyMode.Optional, direction=Direction.Output),
            doc="The can count output workspace group for the low-angle bank, provided there is one. "
            "Each workspace in the group is one event slice.",
        )
        self.declareProperty(
            WorkspaceGroupProperty("OutputWorkspaceHABCanCount", "", optional=PropertyMode.Optional, direction=Direction.Output),
            doc="The can count output workspace group for the high-angle bank, provided there is one. "
            "Each workspace in the group is one event slice.",
        )
        self.declareProperty(
            WorkspaceGroupProperty("OutputWorkspaceHABCanNorm", "", optional=PropertyMode.Optional, direction=Direction.Output),
            doc="The can norm output workspace group for the high-angle bank, provided there is one. "
            "Each workspace in the group is one event slice.",
        )

        self.setPropertyGroup("OutputWorkspaceLABCanCount", "Opt Output")
        self.setPropertyGroup("OutputWorkspaceLABCanNorm", "Opt Output")
        self.setPropertyGroup("OutputWorkspaceHABCanCount", "Opt Output")
        self.setPropertyGroup("OutputWorkspaceHABCanNorm", "Opt Output")

    def PyInit(self):
        self._pyinit()
        self.declareProperty("Period", False)
        self.declareProperty("WavelengthRange", False)

    def PyExec(self):
        self._pyexec()

    def do_initial_reduction(self, state, overall_reduction_mode):
        # --------------------------------------------------------------------------------------------------------------
        # Setup initial reduction
        # --------------------------------------------------------------------------------------------------------------
        initial_reduction_alg = create_child_algorithm(self, "SANSReductionCorePreprocess", **{})
        # Decide which core reduction information to run, i.e. HAB, LAB, ALL, MERGED. In the case of ALL and MERGED,
        # the required simple reduction modes need to be run. Normally this is HAB and LAB, future implementations
        # might have more detectors though (or different types)
        reduction_setting_bundles = self._get_reduction_setting_bundles(state, overall_reduction_mode)

        # --------------------------------------------------------------------------------------------------------------
        # Initial Reduction - steps which can be carried out before event slicing
        # --------------------------------------------------------------------------------------------------------------
        intermediate_bundles = []
        for reduction_setting_bundle in reduction_setting_bundles:
            intermediate_bundles.append(run_initial_event_slice_reduction(initial_reduction_alg, reduction_setting_bundle))
        return self._get_slice_reduction_setting_bundles(intermediate_bundles)

    def do_reduction(self, reduction_alg, reduction_setting_bundles, use_optimizations, progress):
        """
        Perform the main reduction
        :param reduction_alg: SANSReductionCoreEventSlice algorithm
        :param reduction_setting_bundles: a list of list containing workspaces to be reduced.
        :param use_optimizations: bool. If true, use can optimizations in reduction
        :param progress: a progress bar
        :return: output_bundles: a list of list containing output workspaces
                 output_parts_bundles: a list of lists containing output workspaces
                 output_transmission_bundles: a list containing transmission workspaces
        """
        output_bundles = []
        output_parts_bundles = []
        output_transmission_bundles = []
        for event_slice_bundles in reduction_setting_bundles:
            # Output bundles and parts bundles need to be separated by a event slices, but grouped by component
            # e.g. [[workspaces for slice1], [workspaces for slice2]]
            slice_bundles = []
            slice_parts_bundles = []
            for slice_bundle in event_slice_bundles:
                progress.report("Running a single reduction ...")
                # We want to make use of optimizations here.
                # If a can workspace has already been reduced with the same can
                # settings and is stored in the ADS, then we should use it
                # (provided the user has optimizations enabled).
                if use_optimizations and slice_bundle.data_type is DataType.CAN:
                    output_bundle, output_parts_bundle, output_transmission_bundle = run_optimized_for_can(
                        reduction_alg, slice_bundle, event_slice_optimisation=True
                    )
                else:
                    output_bundle, output_parts_bundle, output_transmission_bundle = run_core_event_slice_reduction(
                        reduction_alg, slice_bundle
                    )
                slice_bundles.append(output_bundle)
                slice_parts_bundles.append(output_parts_bundle)
                output_transmission_bundles.append(output_transmission_bundle)
            output_bundles.append(slice_bundles)
            output_parts_bundles.append(slice_parts_bundles)

        return output_bundles, output_parts_bundles, output_transmission_bundles

    @staticmethod
    def _reduction_name():
        return "SANSReductionCoreEventSlice"

    @staticmethod
    def _get_slice_bundles(bundle):
        """
        Splits a reduction package object into several reduction package objects if it
        contains several event slice settings

        :param bundle: a EventSliceSettingBundle tuple
        :return: a list of EventSliceSettingBundle tuples where each tuple contains only one event slice.
        """
        slice_bundles = []
        state = bundle.state
        slice_event_info = state.slice
        start_time = slice_event_info.start_time
        end_time = slice_event_info.end_time

        states = []
        for start, end in zip(start_time, end_time):
            state_copy = deepcopy(state)
            slice_event_info = state_copy.slice
            slice_event_info.start_time = [start]
            slice_event_info.end_time = [end]
            states.append(state_copy)

        for state in states:
            new_state = deepcopy(state)
            slice_bundles.append(
                EventSliceSettingBundle(
                    state=new_state,
                    data_type=bundle.data_type,
                    reduction_mode=bundle.reduction_mode,
                    output_parts=bundle.output_parts,
                    scatter_workspace=bundle.scatter_workspace,
                    dummy_mask_workspace=bundle.dummy_mask_workspace,
                    scatter_monitor_workspace=bundle.scatter_monitor_workspace,
                    direct_workspace=bundle.direct_workspace,
                    transmission_workspace=bundle.transmission_workspace,
                )
            )
        return slice_bundles

    def _get_slice_reduction_setting_bundles(self, intermediate_bundles):
        """
        For each workspace bundle we have from the initial reduction (one for each component),
        create a separate bundle for each event slice.
        We group these as a list of lists, with the structure:
        [[component1 for event slice1, c2 for es1,..], [c1 for es2, c2 for es2, ..], ..]

        :param intermediate_bundles: a list of EventSliceSettingBundle objects,
                                     the output from the initial reduction.
        :return: a list of lists of EventSliceSettingBundle objects, one for each component and event slice.
        """
        sliced_bundles = []
        for bundle in intermediate_bundles:
            sliced_bundles.append(self._get_slice_bundles(bundle))

        # We currently have a list containing a list for each component. Each component list contains workspaces
        # split into event slices. We want the inner list to be component-wise splits so we must transpose this.
        return list(map(list, zip(*sliced_bundles)))

    def set_shift_and_scale_output(self, scale_factors, shift_factors):
        create_workspace_alg = create_child_algorithm(self, "CreateWorkspace", **{"DataX": scale_factors, "DataY": shift_factors})
        create_workspace_alg.execute()
        self.setProperty("OutShiftAndScaleFactor", create_workspace_alg.getProperty("OutputWorkspace").value)

    def set_output_workspaces(self, reduction_mode_vs_output_workspaces, reduction_mode_vs_workspace_names):
        """
        Sets the output workspaces which can be HAB, LAB or Merged.

        At this step we also provide a workspace name to the sample logs which can be used later on for saving
        :param reduction_mode_vs_output_workspaces:  map from reduction mode to output workspace
        :param reduction_mode_vs_workspace_names: map from reduction mode to output workspace name
        """
        workspace_group_merged = WorkspaceGroup()
        workspace_group_lab = WorkspaceGroup()
        workspace_group_hab = WorkspaceGroup()
        # Note that this breaks the flexibility that we have established with the reduction mode. We have not hardcoded
        # HAB or LAB anywhere which means that in the future there could be other detectors of relevance. Here we
        # reference HAB and LAB directly since we currently don't want to rely on dynamic properties. See also in PyInit
        for reduction_mode, output_workspaces in list(reduction_mode_vs_output_workspaces.items()):
            workspace_names = reduction_mode_vs_workspace_names[reduction_mode]
            for output_workspace, output_name in zip(output_workspaces, workspace_names):
                if output_workspace is None:
                    continue
                else:
                    AnalysisDataService.addOrReplace(output_name, output_workspace)
                if reduction_mode is ReductionMode.MERGED:
                    workspace_group_merged.addWorkspace(output_workspace)
                elif reduction_mode is ReductionMode.LAB:
                    workspace_group_lab.addWorkspace(output_workspace)
                elif reduction_mode is ReductionMode.HAB:
                    workspace_group_hab.addWorkspace(output_workspace)
                else:
                    raise RuntimeError(
                        "SANSSingleReduction: Cannot set the output workspace. " "The selected reduction mode {0} is unknown.".format(
                            reduction_mode
                        )
                    )
        if workspace_group_merged.size() > 0:
            self.setProperty("OutputWorkspaceMerged", workspace_group_merged)
        if workspace_group_lab.size() > 0:
            self.setProperty("OutputWorkspaceLAB", workspace_group_lab)
        if workspace_group_hab.size() > 0:
            self.setProperty("OutputWorkspaceHAB", workspace_group_hab)

    def set_reduced_can_workspace_on_output(self, output_bundles):
        """
        Sets the reduced can group workspaces on the output properties.
        The reduced can workspaces can be:
        LAB Can or
        HAB Can

        :param output_bundles: a list of output bundles
        """
        workspace_group_lab_can = WorkspaceGroup()
        workspace_group_hab_can = WorkspaceGroup()
        # Find the LAB Can and HAB Can entries if they exist
        for component_bundle in output_bundles:
            for output_bundle in component_bundle:
                if output_bundle.data_type is DataType.CAN:
                    reduction_mode = output_bundle.reduction_mode
                    output_workspace = output_bundle.output_workspace
                    # Make sure that the output workspace is not None which can be the case if there has never been a
                    # can set for the reduction.
                    if output_workspace is not None and not does_can_workspace_exist_on_ads(output_workspace):
                        name = self._get_output_workspace_name(output_bundle.state, output_bundle.reduction_mode, can=True)
                        AnalysisDataService.addOrReplace(name, output_workspace)
                        if reduction_mode is ReductionMode.LAB:
                            workspace_group_lab_can.addWorkspace(output_workspace)
                        elif reduction_mode is ReductionMode.HAB:
                            workspace_group_hab_can.addWorkspace(output_workspace)
                        else:
                            raise RuntimeError(
                                "SANSSingleReduction: The reduction mode {0} should not" " be set with a can.".format(reduction_mode)
                            )
        if workspace_group_lab_can.size() > 0:
            # LAB group workspace is non-empty, so we want to set it as output
            self.setProperty("OutputWorkspaceLABCan", workspace_group_lab_can)
        if workspace_group_hab_can.size() > 0:
            self.setProperty("OutputWorkspaceHABCan", workspace_group_hab_can)

    def set_reduced_can_count_and_norm_on_output(self, output_bundles_parts):
        """
        Sets the reduced can count and norm group workspaces on the output properties.
        The reduced can workspaces can be:
        1. LAB Can Count
        2. LAB Can Norm
        3. HAB Can Count
        4. HAB Can Norm

        :param output_bundles_parts: a list of output bundle parts
        """
        workspace_group_lab_can_count = WorkspaceGroup()
        workspace_group_lab_can_norm = WorkspaceGroup()
        workspace_group_hab_can_count = WorkspaceGroup()
        workspace_group_hab_can_norm = WorkspaceGroup()
        # Find the partial output bundles fo LAB Can and HAB Can if they exist
        for event_slice_bundles in output_bundles_parts:
            for output_bundle_part in event_slice_bundles:
                if output_bundle_part.data_type is DataType.CAN:
                    reduction_mode = output_bundle_part.reduction_mode
                    output_workspace_count = output_bundle_part.output_workspace_count
                    output_workspace_norm = output_bundle_part.output_workspace_norm
                    # Make sure that the output workspace is not None which can be the case if there has never been a
                    # can set for the reduction.
                    if (
                        output_workspace_norm is not None
                        and output_workspace_count is not None
                        and not does_can_workspace_exist_on_ads(output_workspace_norm)
                        and not does_can_workspace_exist_on_ads(output_workspace_count)
                    ):
                        name = self._get_output_workspace_name(output_bundle_part.state, output_bundle_part.reduction_mode)
                        count_name = name + "_hab_can_count"
                        norm_name = name + "_hab_can_norm"
                        AnalysisDataService.addOrReplace(count_name, output_workspace_count)
                        AnalysisDataService.addOrReplace(norm_name, output_workspace_norm)
                        if reduction_mode is ReductionMode.LAB:
                            workspace_group_lab_can_count.addWorkspace(output_workspace_count)
                            workspace_group_lab_can_norm.addWorkspace(output_workspace_norm)
                        elif reduction_mode is ReductionMode.HAB:
                            workspace_group_hab_can_count.addWorkspace(output_workspace_count)
                            workspace_group_hab_can_norm.addWorkspace(output_workspace_norm)
                        else:
                            raise RuntimeError(
                                "SANSSingleReduction: The reduction mode {0} should not" " be set with a partial can.".format(
                                    reduction_mode
                                )
                            )
        if workspace_group_lab_can_count.size() > 0:
            self.setProperty("OutputWorkspaceLABCanCount", workspace_group_lab_can_count)
        if workspace_group_lab_can_norm.size() > 0:
            self.setProperty("OutputWorkspaceLABCanNorm", workspace_group_lab_can_norm)
        if workspace_group_hab_can_count.size() > 0:
            self.setProperty("OutputWorkspaceHABCanCount", workspace_group_hab_can_count)
        if workspace_group_hab_can_norm.size() > 0:
            self.setProperty("OutputWorkspaceHABCanNorm", workspace_group_hab_can_norm)

    def set_can_and_sam_on_output(self, output_bundles):
        """
        Sets the reduced can and sam workspaces.
        These can be:
        1. LAB Can
        2. HAB Can
        3. LAB Sample
        4. HAB Sample
        Cans are also output for optimization, so check for double output.
        :param output_bundles: a list of output_bundles
        """
        workspace_group_lab_can = WorkspaceGroup()
        workspace_group_hab_can = WorkspaceGroup()
        workspace_group_lab_sample = WorkspaceGroup()
        workspace_group_hab_sample = WorkspaceGroup()

        for component_bundle in output_bundles:
            for output_bundle in component_bundle:
                if output_bundle.data_type is DataType.CAN:
                    reduction_mode = output_bundle.reduction_mode
                    output_workspace = output_bundle.output_workspace

                    if output_workspace is not None and not does_can_workspace_exist_on_ads(output_workspace):
                        can_name = self._get_output_workspace_name(output_bundle.state, output_bundle.reduction_mode, can=True)
                        AnalysisDataService.addOrReplace(can_name, output_workspace)
                        if reduction_mode is ReductionMode.LAB:
                            workspace_group_lab_can.addWorkspace(output_workspace)
                        elif reduction_mode is ReductionMode.HAB:
                            workspace_group_hab_can.addWorkspace(output_workspace)
                        else:
                            raise RuntimeError(
                                "SANSSingleReduction: The reduction mode {0} should not" " be set with a can.".format(reduction_mode)
                            )
                elif output_bundle.data_type is DataType.SAMPLE:
                    reduction_mode = output_bundle.reduction_mode
                    output_workspace = output_bundle.output_workspace

                    if output_workspace is not None:
                        sample_name = self._get_output_workspace_name(output_bundle.state, output_bundle.reduction_mode, sample=True)
                        AnalysisDataService.addOrReplace(sample_name, output_workspace)
                        if reduction_mode is ReductionMode.LAB:
                            workspace_group_lab_sample.addWorkspace(output_workspace)
                        elif reduction_mode is ReductionMode.HAB:
                            workspace_group_hab_sample.addWorkspace(output_workspace)
                        else:
                            raise RuntimeError(
                                "SANSSingleReduction: The reduction mode {0} should not" " be set with a sample.".format(reduction_mode)
                            )

        if workspace_group_hab_can.size() > 0:
            self.setProperty("OutputWorkspaceHABCan", workspace_group_hab_can)
        if workspace_group_hab_sample.size() > 0:
            self.setProperty("OutputWorkspaceHABSample", workspace_group_hab_sample)
        if workspace_group_lab_can.size() > 0:
            self.setProperty("OutputWorkspaceLABCan", workspace_group_lab_can)
        if workspace_group_lab_sample.size() > 0:
            self.setProperty("OutputWorkspaceLABSample", workspace_group_lab_sample)

    def set_transmission_workspaces_on_output(self, transmission_bundles, fit_state):
        for transmission_bundle in transmission_bundles:
            fit_performed = fit_state[transmission_bundle.data_type.value].fit_type != FitType.NO_FIT
            calculated_transmission_workspace = transmission_bundle.calculated_transmission_workspace
            unfitted_transmission_workspace = transmission_bundle.unfitted_transmission_workspace
            if transmission_bundle.data_type is DataType.CAN:
                if does_can_workspace_exist_on_ads(calculated_transmission_workspace):
                    # The workspace is cloned here because the transmission runs are diagnostic output so even though
                    # the values already exist they need to be labelled seperately for each reduction.
                    calculated_transmission_workspace = CloneWorkspace(calculated_transmission_workspace, StoreInADS=False)
                if does_can_workspace_exist_on_ads(unfitted_transmission_workspace):
                    unfitted_transmission_workspace = CloneWorkspace(unfitted_transmission_workspace, StoreInADS=False)
                if fit_performed:
                    self.setProperty("OutputWorkspaceCalculatedTransmissionCan", calculated_transmission_workspace)
                self.setProperty("OutputWorkspaceUnfittedTransmissionCan", unfitted_transmission_workspace)
            elif transmission_bundle.data_type is DataType.SAMPLE:
                if fit_performed:
                    self.setProperty("OutputWorkspaceCalculatedTransmission", calculated_transmission_workspace)
                self.setProperty("OutputWorkspaceUnfittedTransmission", unfitted_transmission_workspace)
            else:
                raise RuntimeError(
                    "SANSSingleReduction: The data type {0} should be" " sample or can.".format(transmission_bundle.data_type)
                )

    def _get_workspace_names(self, reduction_mode_vs_workspace_names, output_bundle):
        output_workspace_names = self._get_final_workspace_names(output_bundle)
        for reduction_mode, name in output_workspace_names.items():
            reduction_mode_vs_workspace_names[reduction_mode].append(name)
        return reduction_mode_vs_workspace_names

    def _get_final_workspace_names(self, output_bundles):
        """This method retrieves the workspace names for event sliced final output workspaces.
        :param output_bundles: A set of outputBundles
        :return: a map of ReductionMode vs final output workspace names"""
        reduction_mode_vs_output_bundles = get_reduction_mode_vs_output_bundles(output_bundles)

        # For each reduction mode, we must find the output name of the workspace
        final_output_workspace_names = {}
        for reduction_mode, output_bundles in reduction_mode_vs_output_bundles.items():
            # Find the sample in the data collection
            state, reduction_mode = next(
                (
                    (output_bundle.state, output_bundle.reduction_mode)
                    for output_bundle in output_bundles
                    if output_bundle.data_type == DataType.SAMPLE
                ),
                None,
            )

            # Get the workspace name
            name = self._get_output_workspace_name(state, reduction_mode=reduction_mode)
            final_output_workspace_names.update({reduction_mode: name})

        return final_output_workspace_names

    def _get_merged_workspace_name(self, output_parts_bundle):
        """This method gets the output workspace names for a merged bundle. This only occurs
        if the reduction mode is Merged.
        :param output_parts_bundle: a list of OutputBundles containing workspaces for a single event slice.
        :return: a workspace name
        """
        state = output_parts_bundle[0].state
        return self._get_output_workspace_name(state, reduction_mode=ReductionMode.MERGED)

    def _get_output_workspace_name(
        self, state, reduction_mode=None, data_type=None, can=False, sample=False, transmission=False, fitted=False
    ):
        """
        Get the output names for the sliced workspaces (within the group workspaces, which are already named).

        :param state: a SANS state object
        :param reduction_mode: an optional ReductionMode enum: "HAB", "LAB", "Merged", or "All"
        :param data_type: an optional DataType enum: "Sample" or "Can"
        :param can: optional bool. If true then creating name for a can workspace
        :param sample: optional bool. If true then creating name for a sample workspace. Sample and can cannot both be
                       true
        :param transmission: optional bool. If true then creating name for a transmission workspace
        :param fitted: optional bool. If true then workspace is a fitted transmission workspace, otherwise unfitted
        :return: name of the workspace
        """
        _multi = {
            "event_slice": True,
            "period": self.getProperty("Period").value,
            "wavelength_range": self.getProperty("WavelengthRange").value,
        }

        if not transmission:
            _suffix = ""
            if can:
                if reduction_mode == ReductionMode.HAB:
                    _suffix = "_hab_can"
                elif reduction_mode == ReductionMode.LAB:
                    _suffix = "_lab_can"
            elif sample:
                if reduction_mode == ReductionMode.HAB:
                    _suffix = "_hab_sample"
                elif reduction_mode == ReductionMode.LAB:
                    _suffix = "_lab_sample"
            return get_output_name(state, reduction_mode, True, suffix=_suffix, multi_reduction_type=_multi)[0]
        else:
            return get_transmission_output_name(state, data_type, _multi, fitted)[0]


# Register algorithm with Mantid
AlgorithmFactory.subscribe(SANSSingleReduction)
