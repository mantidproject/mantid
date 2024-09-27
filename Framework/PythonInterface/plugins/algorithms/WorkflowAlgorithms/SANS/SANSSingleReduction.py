# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
# pylint: disable=invalid-name

"""SANSSingleReduction algorithm performs a single reduction."""

from SANSSingleReductionBase import SANSSingleReductionBase

from mantid.api import AlgorithmFactory, PropertyMode, WorkspaceGroup, WorkspaceGroupProperty
from mantid.kernel import Direction, Property
from mantid.simpleapi import CloneWorkspace
from sans_core.algorithm_detail.single_execution import run_core_reduction, run_optimized_for_can
from sans_core.common.enums import DataType, ReductionMode
from sans_core.common.general_functions import does_can_workspace_exist_on_ads
from sans_core.data_objects.sans_workflow_algorithm_outputs import SANSWorkflowAlgorithmOutputs


class SANSSingleReduction(SANSSingleReductionBase):
    def category(self):
        return "SANS\\Reduction"

    def version(self):
        return 1

    def summary(self):
        return "Performs a single reduction of SANS data."

    def _declare_output_properties(self):
        self.declareProperty("OutScaleFactor", defaultValue=Property.EMPTY_DBL, direction=Direction.Output, doc="Applied scale factor.")

        self.declareProperty("OutShiftFactor", defaultValue=Property.EMPTY_DBL, direction=Direction.Output, doc="Applied shift factor.")

        # This breaks our flexibility with the reduction mode. We need to check if we can populate this based on
        # the available reduction modes for the state input. TODO: check if this is possible
        self.declareProperty(
            WorkspaceGroupProperty("OutputWorkspaceLAB", "", optional=PropertyMode.Optional, direction=Direction.Output),
            doc="The output workspace for the low-angle bank.",
        )
        (
            self.declareProperty(
                WorkspaceGroupProperty("OutputWorkspaceHAB", "", optional=PropertyMode.Optional, direction=Direction.Output),
                doc="The output workspace for the high-angle bank.",
            ),
        )
        self.declareProperty(
            WorkspaceGroupProperty("OutputWorkspaceHABScaled", "", optional=PropertyMode.Optional, direction=Direction.Output),
            doc="The scaled output HAB workspace when merging",
        )
        self.declareProperty(
            WorkspaceGroupProperty("OutputWorkspaceMerged", "", optional=PropertyMode.Optional, direction=Direction.Output),
            doc="The output workspace for the merged reduction.",
        )
        self.setPropertyGroup("OutScaleFactor", "Output")
        self.setPropertyGroup("OutShiftFactor", "Output")
        self.setPropertyGroup("OutputWorkspaceLAB", "Output")
        self.setPropertyGroup("OutputWorkspaceHAB", "Output")
        self.setPropertyGroup("OutputWorkspaceHABScaled", "Output")
        self.setPropertyGroup("OutputWorkspaceMerged", "Output")

        # CAN output
        self.declareProperty(
            WorkspaceGroupProperty("OutputWorkspaceLABCan", "", optional=PropertyMode.Optional, direction=Direction.Output),
            doc="The can output workspace for the low-angle bank, provided there is one.",
        )
        self.declareProperty(
            WorkspaceGroupProperty("OutputWorkspaceHABCan", "", optional=PropertyMode.Optional, direction=Direction.Output),
            doc="The can output workspace for the high-angle bank, provided there is one.",
        )
        self.declareProperty(
            WorkspaceGroupProperty("OutputWorkspaceLABSample", "", optional=PropertyMode.Optional, direction=Direction.Output),
            doc="The sample output workspace for the low-angle bank, provided there is one.",
        )
        self.declareProperty(
            WorkspaceGroupProperty("OutputWorkspaceHABSample", "", optional=PropertyMode.Optional, direction=Direction.Output),
            doc="The sample output workspace for the high-angle bank, provided there is one",
        )
        self.declareProperty(
            WorkspaceGroupProperty("OutputWorkspaceCalculatedTransmission", "", optional=PropertyMode.Optional, direction=Direction.Output),
            doc="The calculated transmission workspace",
        )
        self.declareProperty(
            WorkspaceGroupProperty("OutputWorkspaceUnfittedTransmission", "", optional=PropertyMode.Optional, direction=Direction.Output),
            doc="The unfitted transmission workspace",
        )
        self.declareProperty(
            WorkspaceGroupProperty(
                "OutputWorkspaceCalculatedTransmissionCan", "", optional=PropertyMode.Optional, direction=Direction.Output
            ),
            doc="The calculated transmission workspace for the can",
        )
        self.declareProperty(
            WorkspaceGroupProperty(
                "OutputWorkspaceUnfittedTransmissionCan", "", optional=PropertyMode.Optional, direction=Direction.Output
            ),
            doc="The unfitted transmission workspace for the can",
        )
        self.setPropertyGroup("OutputWorkspaceLABCan", "Can Output")
        self.setPropertyGroup("OutputWorkspaceHABCan", "Can Output")
        self.setPropertyGroup("OutputWorkspaceLABSample", "Can Output")
        self.setPropertyGroup("OutputWorkspaceHABSample", "Can Output")

        # Output CAN Count and Norm for optimizations
        self.declareProperty(
            WorkspaceGroupProperty("OutputWorkspaceLABCanNorm", "", optional=PropertyMode.Optional, direction=Direction.Output),
            doc="The can norm output workspace for the low-angle bank, provided there is one.",
        )
        self.declareProperty(
            WorkspaceGroupProperty("OutputWorkspaceLABCanCount", "", optional=PropertyMode.Optional, direction=Direction.Output),
            doc="The can count output workspace for the low-angle bank, provided there is one.",
        )
        self.declareProperty(
            WorkspaceGroupProperty("OutputWorkspaceHABCanCount", "", optional=PropertyMode.Optional, direction=Direction.Output),
            doc="The can count output workspace for the high-angle bank, provided there is one.",
        )
        self.declareProperty(
            WorkspaceGroupProperty("OutputWorkspaceHABCanNorm", "", optional=PropertyMode.Optional, direction=Direction.Output),
            doc="The can norm output workspace for the high-angle bank, provided there is one.",
        )

        self.setPropertyGroup("OutputWorkspaceLABCanCount", "Opt Output")
        self.setPropertyGroup("OutputWorkspaceLABCanNorm", "Opt Output")
        self.setPropertyGroup("OutputWorkspaceHABCanCount", "Opt Output")
        self.setPropertyGroup("OutputWorkspaceHABCanNorm", "Opt Output")

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

    def do_reduction(self, reduction_alg, reduction_setting_bundle, use_optimizations, progress):
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
        progress.report("Running reduction ...")
        # We want to make use of optimizations here. If a can workspace has already been reduced with the same can
        # settings and is stored in the ADS, then we should use it (provided the user has optimizations enabled).
        if use_optimizations and reduction_setting_bundle.data_type is DataType.CAN:
            reduced_slices = run_optimized_for_can(reduction_alg, reduction_setting_bundle)
        else:
            reduced_slices = run_core_reduction(reduction_alg, reduction_setting_bundle)
        return reduced_slices

    def _set_prop_if_group_has_data(self, prop_name, group_ws):
        if group_ws.size() != 0:
            self.setProperty(prop_name, group_ws)

    def set_shift_and_scale_output(self, scale_factors, shift_factors):
        self.setProperty("OutScaleFactor", scale_factors[0])
        self.setProperty("OutShiftFactor", shift_factors[0])

    def set_output_workspaces(self, workflow_outputs: SANSWorkflowAlgorithmOutputs):
        """
        Sets the output workspaces which can be HAB, LAB or Merged.

        At this step we also provide a workspace name to the sample logs which can be used later on for saving
        :param workflow_outputs:  collection of wavelength sliced and reduced workspaces
        """
        # Note that this breaks the flexibility that we have established with the reduction mode. We have not hardcoded
        # HAB or LAB anywhere which means that in the future there could be other detectors of relevance. Here we
        # reference HAB and LAB directly since we currently don't want to rely on dynamic properties. See also in PyInit

        merged, lab, hab, scaled = WorkspaceGroup(), WorkspaceGroup(), WorkspaceGroup(), WorkspaceGroup()

        for ws in workflow_outputs.lab_output:
            lab.addWorkspace(ws)

        for ws in workflow_outputs.hab_output:
            hab.addWorkspace(ws)

        for ws in workflow_outputs.merged_output:
            merged.addWorkspace(ws)

        for ws in workflow_outputs.scaled_hab_output:
            scaled.addWorkspace(ws)

        self._set_prop_if_group_has_data("OutputWorkspaceLAB", lab)
        self._set_prop_if_group_has_data("OutputWorkspaceHAB", hab)
        self._set_prop_if_group_has_data("OutputWorkspaceHABScaled", scaled)
        self._set_prop_if_group_has_data("OutputWorkspaceMerged", merged)

    def set_reduced_can_workspace_on_output(self, completed_event_bundled):
        """
        Sets the reduced can workspaces on the output properties.

        The reduced can workspaces can be either LAB or HAB
        :param completed_event_bundled: a list containing output bundles
        """
        # Find the LAB Can and HAB Can entries if they exist
        lab_groups = WorkspaceGroup()
        hab_groups = WorkspaceGroup()

        for bundle in completed_event_bundled:
            if bundle.output_bundle.data_type is DataType.CAN:
                reduction_mode = bundle.output_bundle.reduction_mode
                output_workspace = bundle.output_bundle.output_workspace
                # Make sure that the output workspace is not None which can be the case if there has never been a
                # can set for the reduction.

                if output_workspace is not None and not does_can_workspace_exist_on_ads(output_workspace):
                    if reduction_mode is ReductionMode.LAB:
                        lab_groups.addWorkspace(output_workspace)
                    elif reduction_mode is ReductionMode.HAB:
                        hab_groups.addWorkspace(output_workspace)
                    else:
                        raise RuntimeError(
                            "SANSSingleReduction: The reduction mode {0} should not" " be set with a can.".format(reduction_mode)
                        )

        self._set_prop_if_group_has_data("OutputWorkspaceLABCan", lab_groups)
        self._set_prop_if_group_has_data("OutputWorkspaceHABCan", hab_groups)

    def set_reduced_can_count_and_norm_on_output(self, completed_event_slices):
        """
        Sets the reduced can count and norm group workspaces on the output properties.
        This includes the HAB/LAB counts and Norms

        :param completed_event_slices: a list containing a single list of output bundle parts
        """
        # Find the partial output bundles fo LAB Can and HAB Can if they exist
        lab_can_counts, hab_can_counts = WorkspaceGroup(), WorkspaceGroup()
        lab_can_norms, hab_can_norms = WorkspaceGroup(), WorkspaceGroup()

        for bundle in completed_event_slices:
            if bundle.output_bundle.data_type is DataType.CAN:
                reduction_mode = bundle.parts_bundle.reduction_mode
                output_workspace_count = bundle.parts_bundle.output_workspace_count
                output_workspace_norm = bundle.parts_bundle.output_workspace_norm
                # Make sure that the output workspace is not None which can be the case if there has never been a
                # can set for the reduction.
                if (
                    output_workspace_norm is not None
                    and output_workspace_count is not None
                    and not does_can_workspace_exist_on_ads(output_workspace_norm)
                    and not does_can_workspace_exist_on_ads(output_workspace_count)
                ):
                    if reduction_mode is ReductionMode.LAB:
                        lab_can_counts.addWorkspace(output_workspace_count)
                        lab_can_norms.addWorkspace(output_workspace_norm)
                    elif reduction_mode is ReductionMode.HAB:
                        hab_can_counts.addWorkspace(output_workspace_count)
                        hab_can_norms.addWorkspace(output_workspace_norm)
                    else:
                        raise RuntimeError(
                            "SANSSingleReduction: The reduction mode {0} should not" " be set with a partial can.".format(reduction_mode)
                        )

        self._set_prop_if_group_has_data("OutputWorkspaceLABCanCount", lab_can_counts)
        self._set_prop_if_group_has_data("OutputWorkspaceLABCanNorm", lab_can_norms)
        self._set_prop_if_group_has_data("OutputWorkspaceHABCanCount", hab_can_counts)
        self._set_prop_if_group_has_data("OutputWorkspaceHABCanNorm", hab_can_norms)

    def set_can_and_sam_on_output(self, completed_event_slices):
        """
        Sets the reduced can and sample workspaces.
        These is the LAB/HAB can and sample
        Cans are also output for optimization, so check for double output.
        :param output_bundles: a list containing a single list of output_bundles
        """
        lab_cans, hab_cans = WorkspaceGroup(), WorkspaceGroup()
        lab_samples, hab_samples = WorkspaceGroup(), WorkspaceGroup()
        for bundle in completed_event_slices:
            reduction_mode = bundle.output_bundle.reduction_mode
            output_workspace = bundle.output_bundle.output_workspace
            if bundle.output_bundle.data_type is DataType.CAN:
                if output_workspace is not None and not does_can_workspace_exist_on_ads(output_workspace):
                    if reduction_mode is ReductionMode.LAB:
                        lab_cans.addWorkspace(output_workspace)
                    elif reduction_mode is ReductionMode.HAB:
                        hab_cans.addWorkspace(output_workspace)
                    else:
                        raise RuntimeError(
                            "SANSSingleReduction: The reduction mode {0} should not" " be set with a can.".format(reduction_mode)
                        )
            elif bundle.output_bundle.data_type is DataType.SAMPLE:
                if output_workspace is not None and not does_can_workspace_exist_on_ads(output_workspace):
                    if reduction_mode is ReductionMode.LAB:
                        lab_samples.addWorkspace(output_workspace)
                    elif reduction_mode is ReductionMode.HAB:
                        hab_samples.addWorkspace(output_workspace)
                    else:
                        raise RuntimeError(
                            "SANSSingleReduction: The reduction mode {0} should not" " be set with a sample.".format(reduction_mode)
                        )
        self._set_prop_if_group_has_data("OutputWorkspaceLABCan", lab_cans)
        self._set_prop_if_group_has_data("OutputWorkspaceHABCan", hab_cans)
        self._set_prop_if_group_has_data("OutputWorkspaceLABSample", lab_samples)
        self._set_prop_if_group_has_data("OutputWorkspaceHABSample", hab_samples)

    def set_transmission_workspaces_on_output(self, completed_event_slices, fit_state):
        calc_can, calc_sample = WorkspaceGroup(), WorkspaceGroup()
        unfit_can, unfit_sample = WorkspaceGroup(), WorkspaceGroup()
        output_hab_or_lab = None
        for bundle in completed_event_slices:
            if output_hab_or_lab is not None and output_hab_or_lab != bundle.output_bundle.reduction_mode:
                continue  # The transmission workspace for HAB/LAB is the same, so only output one
            output_hab_or_lab = bundle.output_bundle.reduction_mode
            calculated_transmission_workspace = bundle.transmission_bundle.calculated_transmission_workspace
            unfitted_transmission_workspace = bundle.transmission_bundle.unfitted_transmission_workspace
            if bundle.transmission_bundle.data_type is DataType.CAN:
                if does_can_workspace_exist_on_ads(calculated_transmission_workspace):
                    # The workspace is cloned here because the transmission runs are diagnostic output so even though
                    # the values already exist they need to be labelled separately for each reduction.
                    calculated_transmission_workspace = CloneWorkspace(calculated_transmission_workspace, StoreInADS=False)
                if does_can_workspace_exist_on_ads(unfitted_transmission_workspace):
                    unfitted_transmission_workspace = CloneWorkspace(unfitted_transmission_workspace, StoreInADS=False)
                if calculated_transmission_workspace:
                    calc_can.addWorkspace(calculated_transmission_workspace)
                if unfitted_transmission_workspace:
                    unfit_can.addWorkspace(unfitted_transmission_workspace)

            elif bundle.transmission_bundle.data_type is DataType.SAMPLE:
                if calculated_transmission_workspace:
                    calc_sample.addWorkspace(calculated_transmission_workspace)
                if unfitted_transmission_workspace:
                    unfit_sample.addWorkspace(unfitted_transmission_workspace)
            else:
                raise RuntimeError(
                    "SANSSingleReduction: The data type {0} should be" " sample or can.".format(bundle.transmission_bundle.data_type)
                )

        self._set_prop_if_group_has_data("OutputWorkspaceCalculatedTransmission", calc_sample)
        self._set_prop_if_group_has_data("OutputWorkspaceUnfittedTransmission", unfit_sample)
        self._set_prop_if_group_has_data("OutputWorkspaceCalculatedTransmissionCan", calc_can)
        self._set_prop_if_group_has_data("OutputWorkspaceUnfittedTransmissionCan", unfit_can)

    def _get_workspace_names(self, reduction_mode_vs_workspace_names, _):
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
