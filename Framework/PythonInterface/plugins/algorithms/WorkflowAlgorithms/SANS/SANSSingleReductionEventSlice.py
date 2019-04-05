# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
# pylint: disable=invalid-name

""" SANSSingleReductionEventSlice algorithm performs a single reduction on event sliced data."""

from __future__ import (absolute_import, division, print_function)

from mantid.api import (DistributedDataProcessorAlgorithm, MatrixWorkspaceProperty, AlgorithmFactory, PropertyMode,
                        Progress, WorkspaceGroupProperty)
from mantid.kernel import (Direction, PropertyManagerProperty, Property)
from mantid.simpleapi import CloneWorkspace
from sans.algorithm_detail.bundles import ReductionSettingBundle
from sans.algorithm_detail.single_execution import (run_initial_event_slice_reduction, run_core_reduction,
                                                    get_final_output_workspaces,
                                                    get_merge_bundle_for_merge_request, run_optimized_for_can)
from sans.algorithm_detail.strip_end_nans_and_infs import strip_end_nans
from sans.common.enums import (ReductionMode, DataType, ISISReductionMode, FitType)
from sans.common.general_functions import (create_child_algorithm, does_can_workspace_exist_on_ads)
from sans.state.state_base import create_deserialized_sans_state_from_property_manager


class SANSSingleReductionEventSlice(DistributedDataProcessorAlgorithm):
    def category(self):
        return 'SANS\\Reduction'

    def summary(self):
        return 'Performs a single reduction of SANS data which has event slices.'

    def PyInit(self):
        # ----------
        # INPUT
        # ----------
        self.declareProperty(PropertyManagerProperty('SANSState'),
                             doc='A property manager which fulfills the SANSState contract.')

        self.declareProperty("UseOptimizations", True, direction=Direction.Input,
                             doc="When enabled the ADS is being searched for already loaded and reduced workspaces. "
                                 "Depending on your concrete reduction, this could provide a significant"
                                 " performance boost")

        self.declareProperty("SaveCan", False, direction=Direction.Input,
                             doc="When enabled, the unsubtracted can and sam workspaces are added to the ADS.")

        # Sample Scatter Workspaces
        self.declareProperty(MatrixWorkspaceProperty('SampleScatterWorkspace', '',
                                                     optional=PropertyMode.Mandatory, direction=Direction.Input),
                             doc='The sample scatter workspace. This workspace does not contain monitors.')
        self.declareProperty(MatrixWorkspaceProperty('SampleScatterMonitorWorkspace', '',
                                                     optional=PropertyMode.Mandatory, direction=Direction.Input),
                             doc='The sample scatter monitor workspace. This workspace only contains monitors.')

        # Sample Transmission Workspace
        self.declareProperty(MatrixWorkspaceProperty('SampleTransmissionWorkspace', '',
                                                     optional=PropertyMode.Optional, direction=Direction.Input),
                             doc='The sample transmission workspace.')

        # Sample Direct Workspace
        self.declareProperty(MatrixWorkspaceProperty('SampleDirectWorkspace', '',
                                                     optional=PropertyMode.Optional, direction=Direction.Input),
                             doc='The sample scatter direct workspace.')

        self.setPropertyGroup("SampleScatterWorkspace", 'Sample')
        self.setPropertyGroup("SampleScatterMonitorWorkspace", 'Sample')
        self.setPropertyGroup("SampleTransmissionWorkspace", 'Sample')
        self.setPropertyGroup("SampleDirectWorkspace", 'Sample')

        # Can Scatter Workspaces
        self.declareProperty(MatrixWorkspaceProperty('CanScatterWorkspace', '',
                                                     optional=PropertyMode.Optional, direction=Direction.Input),
                             doc='The can scatter workspace. This workspace does not contain monitors.')
        self.declareProperty(MatrixWorkspaceProperty('CanScatterMonitorWorkspace', '',
                                                     optional=PropertyMode.Optional, direction=Direction.Input),
                             doc='The can scatter monitor workspace. This workspace only contains monitors.')

        # Sample Transmission Workspace
        self.declareProperty(MatrixWorkspaceProperty('CanTransmissionWorkspace', '',
                                                     optional=PropertyMode.Optional, direction=Direction.Input),
                             doc='The can transmission workspace.')

        # Sample Direct Workspace
        self.declareProperty(MatrixWorkspaceProperty('CanDirectWorkspace', '',
                                                     optional=PropertyMode.Optional, direction=Direction.Input),
                             doc='The sample scatter direct workspace.')

        self.setPropertyGroup("CanScatterWorkspace", 'Can')
        self.setPropertyGroup("CanScatterMonitorWorkspace", 'Can')
        self.setPropertyGroup("CanTransmissionWorkspace", 'Can')
        self.setPropertyGroup("CanDirectWorkspace", 'Can')

        # ----------
        # OUTPUT
        # ----------
        self.declareProperty('OutScaleFactor', defaultValue=Property.EMPTY_DBL, direction=Direction.Output,
                             doc='Applied scale factor.')

        self.declareProperty('OutShiftFactor', defaultValue=Property.EMPTY_DBL, direction=Direction.Output,
                             doc='Applied shift factor.')

        # This breaks our flexibility with the reduction mode. We need to check if we can populate this based on
        # the available reduction modes for the state input. TODO: check if this is possible
        self.declareProperty(WorkspaceGroupProperty('OutputWorkspaceLAB', '',
                                                    optional=PropertyMode.Optional, direction=Direction.Output),
                             doc='The output workspace for the low-angle bank.')
        self.declareProperty(WorkspaceGroupProperty('OutputWorkspaceHAB', '',
                                                    optional=PropertyMode.Optional, direction=Direction.Output),
                             doc='The output workspace for the high-angle bank.')
        self.declareProperty(WorkspaceGroupProperty('OutputWorkspaceMerged', '',
                                                    optional=PropertyMode.Optional, direction=Direction.Output),
                             doc='The output workspace for the merged reduction.')
        self.setPropertyGroup("OutScaleFactor", 'Output')
        self.setPropertyGroup("OutShiftFactor", 'Output')
        self.setPropertyGroup("OutputWorkspaceLAB", 'Output')
        self.setPropertyGroup("OutputWorkspaceHAB", 'Output')
        self.setPropertyGroup("OutputWorkspaceMerged", 'Output')

        # CAN output
        self.declareProperty(WorkspaceGroupProperty('OutputWorkspaceLABCan', '',
                                                    optional=PropertyMode.Optional, direction=Direction.Output),
                             doc='The can output workspace group for the low-angle bank, provided there is one. '
                                 'Each workspace in the group is one event slice.')
        self.declareProperty(WorkspaceGroupProperty('OutputWorkspaceHABCan', '',
                                                    optional=PropertyMode.Optional, direction=Direction.Output),
                             doc='The can output workspace group for the high-angle bank, provided there is one. '
                                 'Each workspace in the group is one event slice.')
        self.declareProperty(WorkspaceGroupProperty('OutputWorkspaceLABSample', '',
                                                    optional=PropertyMode.Optional, direction=Direction.Output),
                             doc='The sample output workspace group for the low-angle bank, provided there is one. '
                                 'Each workspace in the group is one event slice.')
        self.declareProperty(WorkspaceGroupProperty('OutputWorkspaceHABSample', '',
                                                    optional=PropertyMode.Optional, direction=Direction.Output),
                             doc='The sample output workspace group for the high-angle bank, provided there is one. '
                                 'Each workspace in the group is one event slice.')
        self.declareProperty(WorkspaceGroupProperty('OutputWorkspaceCalculatedTransmission', '',
                                                    optional=PropertyMode.Optional, direction=Direction.Output),
                             doc='The calculated transmission workspace group. '
                                 'Each workspace in the group is one event slice.')
        self.declareProperty(WorkspaceGroupProperty('OutputWorkspaceUnfittedTransmission', '',
                                                    optional=PropertyMode.Optional, direction=Direction.Output),
                             doc='The unfitted transmission workspace group. '
                                 'Each workspace in the group is one event slice.')
        self.declareProperty(WorkspaceGroupProperty('OutputWorkspaceCalculatedTransmissionCan', '',
                                                    optional=PropertyMode.Optional, direction=Direction.Output),
                             doc='The calculated transmission workspace group for the can. '
                                 'Each workspace in the group is one event slice.')
        self.declareProperty(WorkspaceGroupProperty('OutputWorkspaceUnfittedTransmissionCan', '',
                                                    optional=PropertyMode.Optional, direction=Direction.Output),
                             doc='The unfitted transmission workspace group for the can. '
                                 'Each workspace in the group is one event slice.')
        self.setPropertyGroup("OutputWorkspaceLABCan", 'Can Output')
        self.setPropertyGroup("OutputWorkspaceHABCan", 'Can Output')
        self.setPropertyGroup("OutputWorkspaceLABSample", 'Can Output')
        self.setPropertyGroup("OutputWorkspaceHABSample", 'Can Output')

        # Output CAN Count and Norm for optimizations
        self.declareProperty(WorkspaceGroupProperty('OutputWorkspaceLABCanNorm', '',
                                                    optional=PropertyMode.Optional, direction=Direction.Output),
                             doc='The can norm output workspace group for the low-angle bank, provided there is one. '
                                 'Each workspace in the group is one event slice.')
        self.declareProperty(WorkspaceGroupProperty('OutputWorkspaceLABCanCount', '',
                                                    optional=PropertyMode.Optional, direction=Direction.Output),
                             doc='The can count output workspace group for the low-angle bank, provided there is one. '
                                 'Each workspace in the group is one event slice.')
        self.declareProperty(WorkspaceGroupProperty('OutputWorkspaceHABCanCount', '',
                                                    optional=PropertyMode.Optional, direction=Direction.Output),
                             doc='The can count output workspace group for the high-angle bank, provided there is one. '
                                 'Each workspace in the group is one event slice.')
        self.declareProperty(WorkspaceGroupProperty('OutputWorkspaceHABCanNorm', '',
                                                    optional=PropertyMode.Optional, direction=Direction.Output),
                             doc='The can norm output workspace group for the high-angle bank, provided there is one. '
                                 'Each workspace in the group is one event slice.')

        self.setPropertyGroup("OutputWorkspaceLABCanCount", 'Opt Output')
        self.setPropertyGroup("OutputWorkspaceLABCanNorm", 'Opt Output')
        self.setPropertyGroup("OutputWorkspaceHABCanCount", 'Opt Output')
        self.setPropertyGroup("OutputWorkspaceHABCanNorm", 'Opt Output')

    def PyExec(self):
        # Get state
        state = self._get_state()

        # Get reduction mode
        overall_reduction_mode = self._get_reduction_mode(state)

        # --------------------------------------------------------------------------------------------------------------
        # Setup initial reduction
        # --------------------------------------------------------------------------------------------------------------
        # TODO progress bar
        initial_reduction_alg = create_child_algorithm(self, "SANSReductionCoreInitial", **{})
        # Decide which core reduction information to run, i.e. HAB, LAB, ALL, MERGED. In the case of ALL and MERGED,
        # the required simple reduction modes need to be run. Normally this is HAB and LAB, future implementations
        # might have more detectors though (or different types)
        reduction_setting_bundles = self._get_initial_reduction_setting_bundles(state, overall_reduction_mode)

        # --------------------------------------------------------------------------------------------------------------
        # Initial Reduction - steps which can be carried out before event slicing
        # --------------------------------------------------------------------------------------------------------------
        intermediate_bundles = []
        for reduction_setting_bundle in reduction_setting_bundles:
            intermediate_bundles.append(run_initial_event_slice_reduction(initial_reduction_alg,
                                                                          reduction_setting_bundle))

        # --------------------------------------------------------------------------------------------------------------
        # Setup sliced reduction
        # --------------------------------------------------------------------------------------------------------------
        # TODO do we use this state or get the state from the intermediate bundles
        slice_reduction_setting_bundles = self._get_slice_reduction_setting_bundles(state, intermediate_bundles)

        # Run core reductions
        use_optimizations = self.getProperty("UseOptimizations").value
        save_can = self.getProperty("SaveCan").value

        # Create the reduction core algorithm
        reduction_alg = create_child_algorithm(self, "SANSReductionCoreEventSlice", **{})

        # Set up progress
        progress = self._get_progress(len(reduction_setting_bundles), overall_reduction_mode)

        # --------------------------------------------------------------------------------------------------------------
        # Sliced Reduction - here we slice the workspaces and perform the steps which must be carried out after slicing
        # --------------------------------------------------------------------------------------------------------------
        output_bundles = []
        output_parts_bundles = []
        output_transmission_bundles = []
        for slice_reduction_setting_bundle in slice_reduction_setting_bundles:
            progress.report("Running a single reduction ...")
            # We want to make use of optimizations here. If a can workspace has already been reduced with the same can
            # settings and is stored in the ADS, then we should use it (provided the user has optimizations enabled).
            if use_optimizations and slice_reduction_setting_bundle.data_type is DataType.Can:
                output_bundle, output_parts_bundle, \
                    output_transmission_bundle = run_optimized_for_can(reduction_alg, slice_reduction_setting_bundle)
            else:
                output_bundle, output_parts_bundle, \
                    output_transmission_bundle = run_core_reduction(reduction_alg, slice_reduction_setting_bundle)
            output_bundles.append(output_bundle)
            output_parts_bundles.append(output_parts_bundle)
            output_transmission_bundles.append(output_transmission_bundle)

        # TODO fix the output!
        reduction_mode_vs_output_workspaces = {}

        # --------------------------------------------------------------------------------------------------------------
        # Deal with non-merged
        # Note that we have non-merged workspaces even in the case of a merged reduction, ie LAB and HAB results
        # --------------------------------------------------------------------------------------------------------------
        progress.report("Final clean up...")
        output_workspaces_non_merged = get_final_output_workspaces(output_bundles, self)
        reduction_mode_vs_output_workspaces.update(output_workspaces_non_merged)

        # --------------------------------------------------------------------------------------------------------------
        # Deal with merging
        # --------------------------------------------------------------------------------------------------------------
        # Merge if required with stitching etc.
        if overall_reduction_mode is ReductionMode.Merged:
            progress.report("Merging reductions ...")
            merge_bundle = get_merge_bundle_for_merge_request(output_parts_bundles, self)
            self.set_shift_and_scale_output(merge_bundle)
            reduction_mode_vs_output_workspaces.update({ReductionMode.Merged: merge_bundle.merged_workspace})
            scaled_HAB = strip_end_nans(merge_bundle.scaled_hab_workspace, self)
            reduction_mode_vs_output_workspaces.update({ISISReductionMode.HAB: scaled_HAB})

        # --------------------------------------------------------------------------------------------------------------
        # Set the output workspaces
        # --------------------------------------------------------------------------------------------------------------
        # Set sample logs
        # Todo: Set sample log -> Userfile and unfitted transmission workspace. Should probably set on
        # higher level (SANSBatch)
        # Set the output workspaces
        self.set_output_workspaces(reduction_mode_vs_output_workspaces)

        # --------------------------------------------------------------------------------------------------------------
        # Set the reduced can workspaces on the output if optimizations are
        # enabled. This will allow SANSBatchReduction to add them to the ADS.
        # --------------------------------------------------------------------------------------------------------------
        if use_optimizations:
            self.set_reduced_can_workspace_on_output(output_bundles, output_parts_bundles)

        if save_can:
            self.set_can_and_sam_on_output(output_bundles)

        self.set_transmission_workspaces_on_output(output_transmission_bundles,
                                                   state.adjustment.calculate_transmission.fit)

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
        state_property_manager = self.getProperty("SANSState").value
        state = create_deserialized_sans_state_from_property_manager(state_property_manager)
        state.property_manager = state_property_manager
        return state

    def _get_reduction_mode(self, state):
        reduction_info = state.reduction
        reduction_mode = reduction_info.reduction_mode
        return reduction_mode

    def _get_initial_reduction_setting_bundles(self, state, reduction_mode):
        # We need to output the parts if we request a merged reduction mode. This is necessary for stitching later on.
        output_parts = reduction_mode is ReductionMode.Merged

        # If the reduction mode is MERGED, then we need to make sure that all reductions for that selection
        # are executed, i.e. we need to split it up
        if reduction_mode is ReductionMode.Merged:
            # If we are dealing with a merged reduction we need to know which detectors should be merged.
            reduction_info = state.reduction
            reduction_modes = reduction_info.get_merge_strategy()
        elif reduction_mode is ReductionMode.All:
            reduction_info = state.reduction
            reduction_modes = reduction_info.get_all_reduction_modes()
        else:
            reduction_modes = [reduction_mode]

        # Create the Scatter information
        sample_info = self._create_initial_reduction_bundles_for_data_type(state=state,
                                                                           data_type=DataType.Sample,
                                                                           reduction_modes=reduction_modes,
                                                                           output_parts=output_parts,
                                                                           scatter_name="SampleScatterWorkspace",
                                                                           scatter_monitor_name="SampleScatterMonitorWorkspace",
                                                                           transmission_name="SampleTransmissionWorkspace",
                                                                           direct_name="SampleDirectWorkspace")

        # Create the Can information
        can_info = self._create_initial_reduction_bundles_for_data_type(state=state,
                                                                        data_type=DataType.Can,
                                                                        reduction_modes=reduction_modes,
                                                                        output_parts=output_parts,
                                                                        scatter_name="CanScatterWorkspace",
                                                                        scatter_monitor_name="CanScatterMonitorWorkspace",
                                                                        transmission_name="CanTransmissionWorkspace",
                                                                        direct_name="CanDirectWorkspace")
        reduction_setting_bundles = sample_info

        # Make sure that the can information has at least a scatter and a monitor workspace
        for can_bundle in can_info:
            if can_bundle.scatter_workspace is not None and can_bundle.scatter_monitor_workspace is not None:
                reduction_setting_bundles.append(can_bundle)
        return reduction_setting_bundles

    def _get_slice_reduction_setting_bundles(self, intermediate_bundles):
        """
        For each workspace bundle we have from the initial reduction (one for each component),
        create a separate bundle for each event slice.

        :param intermediate_bundles: a list of ReductionSettingBundle objects,
                                     the output from the initial reduction.
        :return: a list of ReductionSettingBundle objects, one for each component and event slice.
        """
        sliced_bundles = []
        for bundle in intermediate_bundles:
            # TODO use batch executions function for getting event slice bundles
            sliced_bundles.extend(get_slice_bundles(bundle))
        return sliced_bundles

    def _create_initial_reduction_bundles_for_data_type(self, state, data_type, reduction_modes, output_parts,
                                                        scatter_name, scatter_monitor_name,
                                                        transmission_name, direct_name):
        # Get workspaces
        scatter_workspace = self.getProperty(scatter_name).value

        scatter_monitor_workspace = self.getProperty(scatter_monitor_name).value
        transmission_workspace = self.getProperty(transmission_name).value
        direct_workspace = self.getProperty(direct_name).value

        # Iterate over all requested reduction types, i.e. LAB, HAB, ..
        reduction_setting_bundles = []
        for reduction_mode in reduction_modes:
            reduction_setting_bundle = ReductionSettingBundle(state=state,
                                                              data_type=data_type,
                                                              reduction_mode=reduction_mode,
                                                              output_parts=output_parts,
                                                              scatter_workspace=scatter_workspace,
                                                              scatter_monitor_workspace=scatter_monitor_workspace,
                                                              transmission_workspace=transmission_workspace,
                                                              direct_workspace=direct_workspace)
            reduction_setting_bundles.append(reduction_setting_bundle)
        return reduction_setting_bundles

    def _create_slice_reduction_bundles_for_data_type(self):
        pass

    def set_shift_and_scale_output(self, merge_bundle):
        self.setProperty("OutScaleFactor", merge_bundle.scale)
        self.setProperty("OutShiftFactor", merge_bundle.shift)

    def set_output_workspaces(self, reduction_mode_vs_output_workspaces):
        """
        Sets the output workspaces which can be HAB, LAB or Merged.

        At this step we also provide a workspace name to the sample logs which can be used later on for saving
        :param reduction_mode_vs_output_workspaces:  map from reduction mode to output workspace
        """
        # Note that this breaks the flexibility that we have established with the reduction mode. We have not hardcoded
        # HAB or LAB anywhere which means that in the future there could be other detectors of relevance. Here we
        # reference HAB and LAB directly since we currently don't want to rely on dynamic properties. See also in PyInit
        for reduction_mode, output_workspace in list(reduction_mode_vs_output_workspaces.items()):
            # In an MPI reduction output_workspace is produced on the master rank, skip others.
            if output_workspace is None:
                continue
            if reduction_mode is ReductionMode.Merged:
                self.setProperty("OutputWorkspaceMerged", output_workspace)
            elif reduction_mode is ISISReductionMode.LAB:
                self.setProperty("OutputWorkspaceLAB", output_workspace)
            elif reduction_mode is ISISReductionMode.HAB:
                self.setProperty("OutputWorkspaceHAB", output_workspace)
            else:
                raise RuntimeError("SANSSingleReduction: Cannot set the output workspace. The selected reduction "
                                   "mode {0} is unknown.".format(reduction_mode))

    def set_reduced_can_workspace_on_output(self, output_bundles, output_bundles_part):
        """
        Sets the reduced can workspaces on the output properties.

        The reduced can workspaces can be:
        1. LAB Can
        2. LAB Can Count
        3. LAB Can Norm
        4. HAB Can
        5. HAB Can Count
        6. HAB Can Norm
        :param output_bundles: a list of output bundles
        :param output_bundles_part: a list of partial output bundles
        """
        # Find the LAB Can and HAB Can entries if they exist
        for output_bundle in output_bundles:
            if output_bundle.data_type is DataType.Can:
                reduction_mode = output_bundle.reduction_mode
                output_workspace = output_bundle.output_workspace
                # Make sure that the output workspace is not None which can be the case if there has never been a
                # can set for the reduction.
                if output_workspace is not None and not does_can_workspace_exist_on_ads(output_workspace):
                    if reduction_mode is ISISReductionMode.LAB:
                        self.setProperty("OutputWorkspaceLABCan", output_workspace)
                    elif reduction_mode is ISISReductionMode.HAB:
                        self.setProperty("OutputWorkspaceHABCan", output_bundle.output_workspace)
                    else:
                        raise RuntimeError("SANSSingleReduction: The reduction mode {0} should not"
                                           " be set with a can.".format(reduction_mode))

        # Find the partial output bundles fo LAB Can and HAB Can if they exist
        for output_bundle_part in output_bundles_part:
            if output_bundle_part.data_type is DataType.Can:
                reduction_mode = output_bundle_part.reduction_mode
                output_workspace_count = output_bundle_part.output_workspace_count
                output_workspace_norm = output_bundle_part.output_workspace_norm
                # Make sure that the output workspace is not None which can be the case if there has never been a
                # can set for the reduction.
                if output_workspace_norm is not None and output_workspace_count is not None and \
                        not does_can_workspace_exist_on_ads(output_workspace_norm) and \
                        not does_can_workspace_exist_on_ads(output_workspace_count):
                    if reduction_mode is ISISReductionMode.LAB:
                        self.setProperty("OutputWorkspaceLABCanCount", output_workspace_count)
                        self.setProperty("OutputWorkspaceLABCanNorm", output_workspace_norm)
                    elif reduction_mode is ISISReductionMode.HAB:
                        self.setProperty("OutputWorkspaceHABCanCount", output_workspace_count)
                        self.setProperty("OutputWorkspaceHABCanNorm", output_workspace_norm)
                    else:
                        raise RuntimeError("SANSSingleReduction: The reduction mode {0} should not"
                                           " be set with a partial can.".format(reduction_mode))

    def set_can_and_sam_on_output(self, output_bundles):
        '''
        Sets the reduced can and sam workspaces.
        These can be:
        1. LAB Can
        2. HAB Can
        3. LAB Sample
        4. HAB Sample
        Cans are also output for optimization, so check for double output.
        :param output_bundles: a list of output_bundles
        '''

        for output_bundle in output_bundles:
            if output_bundle.data_type is DataType.Can:
                reduction_mode = output_bundle.reduction_mode
                output_workspace = output_bundle.output_workspace

                if output_workspace is not None and not does_can_workspace_exist_on_ads(output_workspace):
                    if reduction_mode is ISISReductionMode.LAB:
                        self.setProperty("OutputWorkspaceLABCan", output_workspace)
                    elif reduction_mode is ISISReductionMode.HAB:
                        self.setProperty("OutputWorkspaceHABCan", output_bundle.output_workspace)
                    else:
                        raise RuntimeError("SANSSingleReduction: The reduction mode {0} should not"
                                           " be set with a can.".format(reduction_mode))

            elif output_bundle.data_type is DataType.Sample:
                reduction_mode = output_bundle.reduction_mode
                output_workspace = output_bundle.output_workspace

                if output_workspace is not None:
                    if reduction_mode is ISISReductionMode.LAB:
                        self.setProperty("OutputWorkspaceLABSample", output_workspace)
                    elif reduction_mode is ISISReductionMode.HAB:
                        self.setProperty("OutputWorkspaceHABSample", output_bundle.output_workspace)
                    else:
                        raise RuntimeError("SANSSingleReduction: The reduction mode {0} should not"
                                           " be set with a sample.".format(reduction_mode))

    def set_transmission_workspaces_on_output(self, transmission_bundles, fit_state):
        for transmission_bundle in transmission_bundles:
            fit_performed = fit_state[DataType.to_string(transmission_bundle.data_type)].fit_type != FitType.NoFit
            calculated_transmission_workspace = transmission_bundle.calculated_transmission_workspace
            unfitted_transmission_workspace = transmission_bundle.unfitted_transmission_workspace
            if transmission_bundle.data_type is DataType.Can:
                if does_can_workspace_exist_on_ads(calculated_transmission_workspace):
                    # The workspace is cloned here because the transmission runs are diagnostic output so even though
                    # the values already exist they need to be labelled seperately for each reduction.
                    calculated_transmission_workspace = CloneWorkspace(calculated_transmission_workspace, StoreInADS=False)
                if does_can_workspace_exist_on_ads(unfitted_transmission_workspace):
                    unfitted_transmission_workspace = CloneWorkspace(unfitted_transmission_workspace, StoreInADS=False)
                if fit_performed:
                    self.setProperty("OutputWorkspaceCalculatedTransmissionCan", calculated_transmission_workspace)
                self.setProperty("OutputWorkspaceUnfittedTransmissionCan", unfitted_transmission_workspace)
            elif transmission_bundle.data_type is DataType.Sample:
                if fit_performed:
                    self.setProperty("OutputWorkspaceCalculatedTransmission", calculated_transmission_workspace)
                self.setProperty("OutputWorkspaceUnfittedTransmission", unfitted_transmission_workspace)
            else:
                raise RuntimeError("SANSSingleReduction: The data type {0} should be"
                                   " sample or can.".format(transmission_bundle.data_type))

    def _get_progress(self, number_of_reductions, overall_reduction_mode):
        number_from_merge = 1 if overall_reduction_mode is ReductionMode.Merged else 0
        number_of_progress_reports = number_of_reductions + number_from_merge + 1
        return Progress(self, start=0.0, end=1.0, nreports=number_of_progress_reports)


# Register algorithm with Mantid
AlgorithmFactory.subscribe(SANSSingleReductionEventSlice)
