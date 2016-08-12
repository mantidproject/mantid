# pylint: disable=invalid-name

""" SANSSingleReduction algorithm performs a single reduction."""

from mantid.kernel import (Direction, PropertyManagerProperty, FloatArrayProperty,
                           PropertyCriterion, Property)
from mantid.api import (DataProcessorAlgorithm, MatrixWorkspaceProperty, AlgorithmFactory, PropertyMode)

from SANS2.State.SANSStateBase import create_deserialized_sans_state_from_property_manager
from SANS2.Common.SANSEnumerations import (ReductionMode, DataType, OutputParts, ISISReductionMode)
from SANS2.Common.SANSFunctions import create_unmanaged_algorithm
from SANS.Single.SingleExecution import (run_core_reduction, get_final_output_workspaces,
                                         get_merge_bundle_for_merge_request)
from SANS.Single.Bundles import ReductionSettingBundle


class SANSSingleReduction(DataProcessorAlgorithm):
    def category(self):
        return 'SANS\\Reduction'

    def summary(self):
        return 'Performs a single reduction of SANS data.'

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
        self.declareProperty(MatrixWorkspaceProperty('OutputWorkspaceLAB', '',
                                                     optional=PropertyMode.Optional, direction=Direction.Output),
                             doc='The output workspace for the low-angle bank.')
        self.declareProperty(MatrixWorkspaceProperty('OutputWorkspaceHAB', '',
                                                     optional=PropertyMode.Optional, direction=Direction.Output),
                             doc='The output workspace for the high-angle bank.')
        self.declareProperty(MatrixWorkspaceProperty('OutputWorkspaceMerged', '',
                                                     optional=PropertyMode.Optional, direction=Direction.Output),
                             doc='The output workspace for the merged reduction.')

    def PyExec(self):
        # Get state
        state = self._get_state()

        # Get reduction mode
        reduction_mode = self._get_reduction_mode(state)

        # Decide which core reduction information to run, i.e. HAB, LAB, ALL, MERGED. In the case of ALL and MERGED,
        # the required simple reduction modes need to be run. Normally this is HAB and LAB, future implementations
        # might have more detectors though (or different types)
        reduction_setting_bundles = self._get_reduction_setting_bundles(state, reduction_mode)

        # Run core reductions
        use_optimizations = self.getProperty("UseOptimizations").value

        # Create the reduction core algorithm
        reduction_name = "SANSReductionCore"
        reduction_options = {}
        reduction_alg = create_unmanaged_algorithm(reduction_name, **reduction_options)

        # Create containers for the output
        output_bundles = []
        output_parts_bundles = []
        for reduction_setting_bundle in reduction_setting_bundles:

            output_bundle, output_parts_bundle = run_core_reduction(reduction_alg,
                                                                    reduction_setting_bundle,
                                                                    use_optimizations)
            output_bundles.append(output_bundle)
            output_parts_bundles.append(output_parts_bundle)

        reduction_mode_vs_output_workspaces = {}
        # Merge if required with stitching etc.
        if reduction_mode is ReductionMode.Merged:
            merge_bundle = get_merge_bundle_for_merge_request(output_parts_bundles)
            self.set_shift_and_scale_output(merge_bundle)
            reduction_mode_vs_output_workspaces.update({ReductionMode.Merged: merge_bundle.merged_workspace})

        output_workspaces_non_merged = get_final_output_workspaces(output_bundles)
        reduction_mode_vs_output_workspaces.update(output_workspaces_non_merged)

        # Set sample logs
        # Todo: Set sample log
        # Set the output workspaces
        self.set_output_workspaces(reduction_mode_vs_output_workspaces)

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

    def _get_reduction_setting_bundles(self, state, reduction_mode):
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
        sample_info = self._create_reduction_bundles_for_data_type(state=state,
                                                                   data_type=DataType.Sample,
                                                                   reduction_modes=reduction_modes,
                                                                   output_parts=output_parts,
                                                                   scatter_name="SampleScatterWorkspace",
                                                                   scatter_monitor_name="SampleScatterMonitorWorkspace",
                                                                   transmission_name="SampleTransmissionWorkspace",
                                                                   direct_name="SampleDirectWorkspace")

        # Create the Can information
        can_info = self._create_reduction_bundles_for_data_type(state=state,
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

    def _create_reduction_bundles_for_data_type(self, state, data_type, reduction_modes, output_parts,
                                                scatter_name, scatter_monitor_name, transmission_name, direct_name):
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

    def set_shift_and_scale_output(self, merge_bundle):
        self.setProperty("OutScaleFactor", merge_bundle.scale)
        self.setProperty("OutShiftFactor", merge_bundle.shift)

    def set_output_workspaces(self, reduction_mode_vs_output_workspaces):
        # Note that this breaks the flexibility that we have established with the reduction mode. We have not hardcoded
        # HAB or LAB anywhere which means that in the future there could be other detectors of relevance. Here we
        # reference HAB and LAB directly since we currently don't want to rely on dynamic properties. See also in PyInit
        for reduction_mode, output_workspace in reduction_mode_vs_output_workspaces.items():
            if reduction_mode is ReductionMode.Merged:
                self.setProperty("OutputWorkspaceMerged", output_workspace)
            elif reduction_mode is ISISReductionMode.Lab:
                self.setProperty("OutputWorkspaceLAB", output_workspace)
            elif reduction_mode is ISISReductionMode.Hab:
                self.setProperty("OutputWorkspaceLAB", output_workspace)
            else:
                raise RuntimeError("SANSSingleReduction: Cannot set the output workspace. The selected reduction "
                                   "mode {0} is unknown.".format(reduction_mode))


# Register algorithm with Mantid
AlgorithmFactory.subscribe(SANSSingleReduction)
