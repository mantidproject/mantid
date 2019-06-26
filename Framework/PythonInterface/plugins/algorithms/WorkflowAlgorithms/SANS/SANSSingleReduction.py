# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
# pylint: disable=invalid-name

""" SANSSingleReduction algorithm performs a single reduction."""

from __future__ import (absolute_import, division, print_function)

from mantid.api import (MatrixWorkspaceProperty, AlgorithmFactory, PropertyMode)
from mantid.kernel import (Direction, Property)
from mantid.simpleapi import CloneWorkspace
from sans.algorithm_detail.single_execution import (run_core_reduction, run_optimized_for_can)
from sans.common.enums import (ReductionMode, DataType, ISISReductionMode, FitType)
from sans.common.general_functions import does_can_workspace_exist_on_ads

from SANSSingleReductionBase import SANSSingleReductionBase


class SANSSingleReduction(SANSSingleReductionBase):
    def category(self):
        return 'SANS\\Reduction'

    def version(self):
        return 1

    def summary(self):
        return 'Performs a single reduction of SANS data.'

    def _declare_output_properties(self):
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
        self.setPropertyGroup("OutScaleFactor", 'Output')
        self.setPropertyGroup("OutShiftFactor", 'Output')
        self.setPropertyGroup("OutputWorkspaceLAB", 'Output')
        self.setPropertyGroup("OutputWorkspaceHAB", 'Output')
        self.setPropertyGroup("OutputWorkspaceMerged", 'Output')

        # CAN output
        self.declareProperty(MatrixWorkspaceProperty('OutputWorkspaceLABCan', '',
                                                     optional=PropertyMode.Optional, direction=Direction.Output),
                             doc='The can output workspace for the low-angle bank, provided there is one.')
        self.declareProperty(MatrixWorkspaceProperty('OutputWorkspaceHABCan', '',
                                                     optional=PropertyMode.Optional, direction=Direction.Output),
                             doc='The can output workspace for the high-angle bank, provided there is one.')
        self.declareProperty(MatrixWorkspaceProperty('OutputWorkspaceLABSample', '',
                                                     optional=PropertyMode.Optional, direction=Direction.Output),
                             doc='The sample output workspace for the low-angle bank, provided there is one.')
        self.declareProperty(MatrixWorkspaceProperty('OutputWorkspaceHABSample', '',
                                                     optional=PropertyMode.Optional, direction=Direction.Output),
                             doc='The sample output workspace for the high-angle bank, provided there is one')
        self.declareProperty(MatrixWorkspaceProperty('OutputWorkspaceCalculatedTransmission', '',
                                                     optional=PropertyMode.Optional, direction=Direction.Output),
                             doc='The calculated transmission workspace')
        self.declareProperty(MatrixWorkspaceProperty('OutputWorkspaceUnfittedTransmission', '',
                                                     optional=PropertyMode.Optional, direction=Direction.Output),
                             doc='The unfitted transmission workspace')
        self.declareProperty(MatrixWorkspaceProperty('OutputWorkspaceCalculatedTransmissionCan', '',
                                                     optional=PropertyMode.Optional, direction=Direction.Output),
                             doc='The calculated transmission workspace for the can')
        self.declareProperty(MatrixWorkspaceProperty('OutputWorkspaceUnfittedTransmissionCan', '',
                                                     optional=PropertyMode.Optional, direction=Direction.Output),
                             doc='The unfitted transmission workspace for the can')
        self.setPropertyGroup("OutputWorkspaceLABCan", 'Can Output')
        self.setPropertyGroup("OutputWorkspaceHABCan", 'Can Output')
        self.setPropertyGroup("OutputWorkspaceLABSample", 'Can Output')
        self.setPropertyGroup("OutputWorkspaceHABSample", 'Can Output')

        # Output CAN Count and Norm for optimizations
        self.declareProperty(MatrixWorkspaceProperty('OutputWorkspaceLABCanNorm', '',
                                                     optional=PropertyMode.Optional, direction=Direction.Output),
                             doc='The can norm output workspace for the low-angle bank, provided there is one.')
        self.declareProperty(MatrixWorkspaceProperty('OutputWorkspaceLABCanCount', '',
                                                     optional=PropertyMode.Optional, direction=Direction.Output),
                             doc='The can count output workspace for the low-angle bank, provided there is one.')
        self.declareProperty(MatrixWorkspaceProperty('OutputWorkspaceHABCanCount', '',
                                                     optional=PropertyMode.Optional, direction=Direction.Output),
                             doc='The can count output workspace for the high-angle bank, provided there is one.')
        self.declareProperty(MatrixWorkspaceProperty('OutputWorkspaceHABCanNorm', '',
                                                     optional=PropertyMode.Optional, direction=Direction.Output),
                             doc='The can norm output workspace for the high-angle bank, provided there is one.')

        self.setPropertyGroup("OutputWorkspaceLABCanCount", 'Opt Output')
        self.setPropertyGroup("OutputWorkspaceLABCanNorm", 'Opt Output')
        self.setPropertyGroup("OutputWorkspaceHABCanCount", 'Opt Output')
        self.setPropertyGroup("OutputWorkspaceHABCanNorm", 'Opt Output')

    def PyInit(self):
        self._pyinit()

    def PyExec(self):
        self._pyexec()

    @staticmethod
    def _reduction_name():
        return "SANSReductionCore"

    def do_initial_reduction(self, state, overall_reduction_mode):
        """
        Version 1 does not have an initial reduction.
        This method is required for compatibility with version 2.
        This method create bundles for the main reduction.
        """
        return [self._get_reduction_setting_bundles(state, overall_reduction_mode)]

    def do_reduction(self, reduction_alg, reduction_setting_bundles, use_optimizations, progress):
        """
        Perform the main reduction.
        :param reduction_alg: SANSReductionCore algorithm
        :param reduction_setting_bundles: a list of lists containing workspaces to be reduced.
                                          The outer list is for compatibility with version 2
                                          and only contains one inner list
        :param use_optimizations: bool. If true, use can optimizations
        :param progress: a progress bar
        :return: output_bundles: a list containing a single list of output workspaces
                 output_parts_bundles: a list containing a single list of output workspaces
                 output_transmission_bundles: a list containing transmission workspaces
        """
        reduction_setting_bundles = reduction_setting_bundles[0]
        output_bundles = []
        output_parts_bundles = []
        output_transmission_bundles = []
        for reduction_setting_bundle in reduction_setting_bundles:
            progress.report("Running a single reduction ...")
            # We want to make use of optimizations here. If a can workspace has already been reduced with the same can
            # settings and is stored in the ADS, then we should use it (provided the user has optimizations enabled).
            if use_optimizations and reduction_setting_bundle.data_type is DataType.Can:
                output_bundle, output_parts_bundle, \
                    output_transmission_bundle = run_optimized_for_can(reduction_alg, reduction_setting_bundle)
            else:
                output_bundle, output_parts_bundle, \
                    output_transmission_bundle = run_core_reduction(reduction_alg, reduction_setting_bundle)
            output_bundles.append(output_bundle)
            output_parts_bundles.append(output_parts_bundle)
            output_transmission_bundles.append(output_transmission_bundle)
        return [output_bundles], [output_parts_bundles], output_transmission_bundles

    def set_shift_and_scale_output(self, scale_factors, shift_factors):
        self.setProperty("OutScaleFactor", scale_factors[0])
        self.setProperty("OutShiftFactor", shift_factors[0])

    def set_output_workspaces(self, reduction_mode_vs_output_workspaces, reduction_mode_vs_workspace_names):
        """
        Sets the output workspaces which can be HAB, LAB or Merged.

        At this step we also provide a workspace name to the sample logs which can be used later on for saving
        :param reduction_mode_vs_output_workspaces:  map from reduction mode to output workspace
        :param reduction_mode_vs_workspace_names: an unused dict. Required for version 2 compatibility
        """
        # Note that this breaks the flexibility that we have established with the reduction mode. We have not hardcoded
        # HAB or LAB anywhere which means that in the future there could be other detectors of relevance. Here we
        # reference HAB and LAB directly since we currently don't want to rely on dynamic properties. See also in PyInit
        for reduction_mode, output_workspaces in list(reduction_mode_vs_output_workspaces.items()):
            output_workspace = output_workspaces[0]
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

    def set_reduced_can_workspace_on_output(self, output_bundles):
        """
        Sets the reduced can workspaces on the output properties.

        The reduced can workspaces can be:
        1. LAB Can
        4. HAB Can
        :param output_bundles: a list containing a single list of output bundles
        """
        # Find the LAB Can and HAB Can entries if they exist
        output_bundles = output_bundles[0]
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

    def set_reduced_can_count_and_norm_on_output(self, output_bundles_parts):
        """
        Sets the reduced can count and norm group workspaces on the output properties.
        The reduced can workspaces can be:
        1. LAB Can Count
        2. LAB Can Norm
        3. HAB Can Count
        4. HAB Can Norm

        :param output_bundles_parts: a list containing a single list of output bundle parts
        """
        # Find the partial output bundles fo LAB Can and HAB Can if they exist
        output_bundles_part = output_bundles_parts[0]
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
        """
        Sets the reduced can and sam workspaces.
        These can be:
        1. LAB Can
        2. HAB Can
        3. LAB Sample
        4. HAB Sample
        Cans are also output for optimization, so check for double output.
        :param output_bundles: a list containing a single list of output_bundles
        """
        output_bundles = output_bundles[0]
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
transmission_bundle.data_type.name].fit_type != FitType.NoFit
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

    def _get_workspace_names(self, reduction_mode_vs_workspace_names, event_slice_bundle):
        """
        This method is for compatibility with version 2. It is not required for version 1
        """
        return reduction_mode_vs_workspace_names

    def _get_merged_workspace_name(self, event_slice_part_bundle):
        """
        This method is for compatibility with version 2. It is not required for version 1
        """
        return ""

    def _get_output_workspace_name(self, *args, **kwargs):
        """
        This method is for compatibility with version 2. It is not required for version 1
        """
        return ""


# Register algorithm with Mantid
AlgorithmFactory.subscribe(SANSSingleReduction)
