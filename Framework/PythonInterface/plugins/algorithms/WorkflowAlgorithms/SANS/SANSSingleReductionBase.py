# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
# pylint: disable=invalid-name

"""A base class to share functionality between SANSSingleReduction versions."""

from mantid.api import DataProcessorAlgorithm, MatrixWorkspaceProperty, Progress, PropertyMode
from mantid.kernel import Direction
from sans.algorithm_detail.bundles import ReductionSettingBundle, CompletedSlices
from sans.algorithm_detail.single_execution import get_final_output_workspaces, get_merge_bundle_for_merge_request
from sans.algorithm_detail.strip_end_nans_and_infs import strip_end_nans
from SANS.sans.common.enums import DataType, ReductionMode
from SANS.sans.common.general_functions import create_child_algorithm
from sans.data_objects.sans_workflow_algorithm_outputs import SANSWorkflowAlgorithmOutputs
from sans.state.Serializer import Serializer


class SANSSingleReductionBase(DataProcessorAlgorithm):
    def _pyinit(self):
        # ----------
        # INPUT
        # ----------
        self.declareProperty("SANSState", "", doc="A JSON string which fulfills the SANSState contract.")

        self.declareProperty(
            "UseOptimizations",
            True,
            direction=Direction.Input,
            doc="When enabled the ADS is being searched for already loaded and reduced workspaces. "
            "Depending on your concrete reduction, this could provide a significant"
            " performance boost",
        )

        self.declareProperty(
            "SaveCan", False, direction=Direction.Input, doc="When enabled, the unsubtracted can and sam workspaces are added to the ADS."
        )

        # Sample Scatter Workspaces
        self.declareProperty(
            MatrixWorkspaceProperty("SampleScatterWorkspace", "", optional=PropertyMode.Mandatory, direction=Direction.Input),
            doc="The sample scatter workspace. This workspace does not contain monitors.",
        )
        self.declareProperty(
            MatrixWorkspaceProperty("SampleScatterMonitorWorkspace", "", optional=PropertyMode.Mandatory, direction=Direction.Input),
            doc="The sample scatter monitor workspace. This workspace only contains monitors.",
        )

        # Sample Transmission Workspace
        self.declareProperty(
            MatrixWorkspaceProperty("SampleTransmissionWorkspace", "", optional=PropertyMode.Optional, direction=Direction.Input),
            doc="The sample transmission workspace.",
        )

        # Sample Direct Workspace
        self.declareProperty(
            MatrixWorkspaceProperty("SampleDirectWorkspace", "", optional=PropertyMode.Optional, direction=Direction.Input),
            doc="The sample scatter direct workspace.",
        )

        self.setPropertyGroup("SampleScatterWorkspace", "Sample")
        self.setPropertyGroup("SampleScatterMonitorWorkspace", "Sample")
        self.setPropertyGroup("SampleTransmissionWorkspace", "Sample")
        self.setPropertyGroup("SampleDirectWorkspace", "Sample")

        # Can Scatter Workspaces
        self.declareProperty(
            MatrixWorkspaceProperty("CanScatterWorkspace", "", optional=PropertyMode.Optional, direction=Direction.Input),
            doc="The can scatter workspace. This workspace does not contain monitors.",
        )
        self.declareProperty(
            MatrixWorkspaceProperty("CanScatterMonitorWorkspace", "", optional=PropertyMode.Optional, direction=Direction.Input),
            doc="The can scatter monitor workspace. This workspace only contains monitors.",
        )

        # Sample Transmission Workspace
        self.declareProperty(
            MatrixWorkspaceProperty("CanTransmissionWorkspace", "", optional=PropertyMode.Optional, direction=Direction.Input),
            doc="The can transmission workspace.",
        )

        # Sample Direct Workspace
        self.declareProperty(
            MatrixWorkspaceProperty("CanDirectWorkspace", "", optional=PropertyMode.Optional, direction=Direction.Input),
            doc="The sample scatter direct workspace.",
        )

        self.setPropertyGroup("CanScatterWorkspace", "Can")
        self.setPropertyGroup("CanScatterMonitorWorkspace", "Can")
        self.setPropertyGroup("CanTransmissionWorkspace", "Can")
        self.setPropertyGroup("CanDirectWorkspace", "Can")

        # ----------
        # OUTPUT
        # ----------
        self._declare_output_properties()

    def _pyexec(self):
        # Get state
        state = self._get_state()

        # Get reduction mode
        overall_reduction_mode = self._get_reduction_mode(state)

        # --------------------------------------------------------------------------------------------------------------
        # Perform the initial reduction. Version 1 does not have an initial reduction.
        # --------------------------------------------------------------------------------------------------------------
        reduction_setting_bundles = self.do_initial_reduction(state, overall_reduction_mode)

        # --------------------------------------------------------------------------------------------------------------
        # Setup main reduction
        # --------------------------------------------------------------------------------------------------------------

        # Run core reductions
        use_optimizations = self.getProperty("UseOptimizations").value
        save_can = self.getProperty("SaveCan").value

        # Create the reduction core algorithm
        reduction_alg = create_child_algorithm(self, self._reduction_name(), **{})

        # Set up progress
        progress = self._get_progress(sum([len(event_list) for event_list in reduction_setting_bundles]), overall_reduction_mode)

        # --------------------------------------------------------------------------------------------------------------
        # Reduction - here we slice the workspaces and perform the steps which must be carried out after slicing
        # --------------------------------------------------------------------------------------------------------------
        completed_event_slices: CompletedSlices = []
        for event_slice in reduction_setting_bundles:
            # The single reductions represent CAN / sample reductions
            for bundle in event_slice:
                reduced_slices = self.do_reduction(reduction_alg, bundle, use_optimizations, progress)
                # Merge the list of lists into a single flat list to keep our lives easier
                completed_event_slices.extend(reduced_slices)

        # --------------------------------------------------------------------------------------------------------------
        # Deal with non-merged
        # Note that we have non-merged workspaces even in the case of a merged reduction, ie LAB and HAB results
        # --------------------------------------------------------------------------------------------------------------
        progress.report("Final clean up...")

        workflow_alg_outputs = get_final_output_workspaces(completed_event_slices, self)

        # --------------------------------------------------------------------------------------------------------------
        # Deal with merging
        # --------------------------------------------------------------------------------------------------------------
        # Merge if required with stitching etc.
        scale_factors = []
        shift_factors = []

        if overall_reduction_mode is ReductionMode.MERGED:
            progress.report("Merging reductions ...")
            merge_bundle = get_merge_bundle_for_merge_request(completed_event_slices, self)
            for merged in merge_bundle:
                scale_factors.append(merged.scale)
                shift_factors.append(merged.shift)
                workflow_alg_outputs.merged_output.append(merged.merged_workspace)
                # Pack scaled HAB as a diagnostic tool
                scaled_HAB = strip_end_nans(merged.scaled_hab_workspace, self)
                workflow_alg_outputs.scaled_hab_output.append(scaled_HAB)

            self.set_shift_and_scale_output(scale_factors, shift_factors)

        # --------------------------------------------------------------------------------------------------------------
        # Set the output workspaces
        # --------------------------------------------------------------------------------------------------------------
        self.set_output_workspaces(workflow_alg_outputs)

        # --------------------------------------------------------------------------------------------------------------
        # Set the reduced can workspaces on the output if optimizations are
        # enabled. This will allow SANSBatchReduction to add them to the ADS.
        # --------------------------------------------------------------------------------------------------------------
        if use_optimizations:
            if save_can:
                self.set_reduced_can_workspace_on_output(completed_event_slices)
            self.set_reduced_can_count_and_norm_on_output(completed_event_slices)

        if save_can:
            self.set_can_and_sam_on_output(completed_event_slices)

        self.set_transmission_workspaces_on_output(completed_event_slices, state.adjustment.calculate_transmission.fit)

    def do_initial_reduction(self, state, overall_reduction_mode):
        raise NotImplementedError("do_initial_reduction must be implemented.")

    def _get_workspace_names(self, reduction_mode_vs_workspace_names, event_slice_bundle):
        raise NotImplementedError("_get_workspace_names must be implemented.")

    def _get_merged_workspace_name(self, event_slice_part_bundle):
        raise NotImplementedError("_get_merged_workspace_name must be implemented.")

    def _get_output_workspace_name(
        self, state, reduction_mode=None, data_type=None, can=False, sample=False, transmission=False, fitted=False
    ):
        raise NotImplementedError("_get_output_workspace_name must be implemented.")

    def set_reduced_can_workspace_on_output(self, output_bundles):
        raise NotImplementedError("set_reduced_can_workspace_on_output must be implemented.")

    def set_reduced_can_count_and_norm_on_output(self, output_parts_bundles):
        raise NotImplementedError("set_reduced_can_count_and_norm_on_output must be implemented.")

    def set_can_and_sam_on_output(self, output_bundles):
        raise NotImplementedError("set_can_and_sam_on_output.")

    def set_transmission_workspaces_on_output(self, output_transmission_bundles, fit_state):
        raise NotImplementedError("set_transmission_workspaces_on_output must be implemented.")

    def set_shift_and_scale_output(self, scale_factors, shift_factors):
        raise NotImplementedError("set_shift_and_scale_output must be implemented.")

    def set_output_workspaces(self, workflow_outputs: SANSWorkflowAlgorithmOutputs):
        raise NotImplementedError("set_output_workspaces must be implemented.")

    def do_reduction(self, reduction_alg, reduction_setting_bundles, use_optimizations, progress):
        raise NotImplementedError("do_reduction must be implemented.")

    def validateInputs(self):
        errors = dict()
        # Check that the input can be converted into the right state object
        try:
            state = self._get_state()
            state.validate()
        except ValueError as err:
            errors.update({"SANSSingleReduction": str(err)})
        return errors

    def _get_state(self):
        state_json = self.getProperty("SANSState").value
        state = Serializer.from_json(state_json)

        return state

    @staticmethod
    def _get_reduction_mode(state):
        reduction_info = state.reduction
        reduction_mode = reduction_info.reduction_mode
        return reduction_mode

    def _get_reduction_setting_bundles(self, state, reduction_mode):
        # We need to output the parts if we request a merged reduction mode. This is necessary for stitching later on.
        output_parts = reduction_mode is ReductionMode.MERGED

        # If the reduction mode is MERGED, then we need to make sure that all reductions for that selection
        # are executed, i.e. we need to split it up
        if reduction_mode is ReductionMode.MERGED:
            # If we are dealing with a merged reduction we need to know which detectors should be merged.
            reduction_info = state.reduction
            reduction_modes = reduction_info.get_merge_strategy()
        elif reduction_mode is ReductionMode.ALL:
            reduction_info = state.reduction
            reduction_modes = reduction_info.get_all_reduction_modes()
        else:
            reduction_modes = [reduction_mode]

        # Create the Scatter information
        sample_info = self._create_reduction_bundles_for_data_type(
            state=state,
            data_type=DataType.SAMPLE,
            reduction_modes=reduction_modes,
            output_parts=output_parts,
            scatter_name="SampleScatterWorkspace",
            scatter_monitor_name="SampleScatterMonitorWorkspace",
            transmission_name="SampleTransmissionWorkspace",
            direct_name="SampleDirectWorkspace",
        )

        # Create the Can information
        can_info = self._create_reduction_bundles_for_data_type(
            state=state,
            data_type=DataType.CAN,
            reduction_modes=reduction_modes,
            output_parts=output_parts,
            scatter_name="CanScatterWorkspace",
            scatter_monitor_name="CanScatterMonitorWorkspace",
            transmission_name="CanTransmissionWorkspace",
            direct_name="CanDirectWorkspace",
        )
        reduction_setting_bundles = sample_info

        # Make sure that the can information has at least a scatter and a monitor workspace
        for can_bundle in can_info:
            if can_bundle.scatter_workspace is not None and can_bundle.scatter_monitor_workspace is not None:
                reduction_setting_bundles.append(can_bundle)
        return reduction_setting_bundles

    def _create_reduction_bundles_for_data_type(
        self, state, data_type, reduction_modes, output_parts, scatter_name, scatter_monitor_name, transmission_name, direct_name
    ):
        # Get workspaces
        scatter_workspace = self.getProperty(scatter_name).value

        scatter_monitor_workspace = self.getProperty(scatter_monitor_name).value
        transmission_workspace = self.getProperty(transmission_name).value
        direct_workspace = self.getProperty(direct_name).value

        # Iterate over all requested reduction types, i.e. LAB, HAB, ..
        reduction_setting_bundles = []
        for reduction_mode in reduction_modes:
            reduction_setting_bundle = ReductionSettingBundle(
                state=state,
                data_type=data_type,
                reduction_mode=reduction_mode,
                output_parts=output_parts,
                scatter_workspace=scatter_workspace,
                scatter_monitor_workspace=scatter_monitor_workspace,
                transmission_workspace=transmission_workspace,
                direct_workspace=direct_workspace,
            )
            reduction_setting_bundles.append(reduction_setting_bundle)
        return reduction_setting_bundles

    def _get_progress(self, number_of_reductions, overall_reduction_mode):
        number_from_merge = 1 if overall_reduction_mode is ReductionMode.MERGED else 0
        number_of_progress_reports = number_of_reductions + number_from_merge + 1
        return Progress(self, start=0.0, end=1.0, nreports=number_of_progress_reports)
