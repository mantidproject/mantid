# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
"""Merges two reduction types to single reduction"""

from abc import ABCMeta, abstractmethod

import mantid.simpleapi as mantid_api
from sans.algorithm_detail.bundles import MergeBundle
from SANS.sans.common.enums import SANSFacility, DataType
from SANS.sans.common.general_functions import create_child_algorithm


class Merger(metaclass=ABCMeta):
    """Merger interface"""

    @abstractmethod
    def merge(self, reduction_mode_vs_output_bundles, parent_alg=None):
        pass


class ISIS1DMerger(Merger):
    """
    Class which handles ISIS-style merges.
    """

    def __init__(self):
        super(ISIS1DMerger, self).__init__()

    def calculate_scaled_hab_output(
        self, shift, scale, sample_count_secondary, sample_norm_secondary, can_count_secondary, can_norm_secondary
    ):
        scaled_norm_front = mantid_api.Scale(
            InputWorkspace=sample_norm_secondary, Factor=1.0 / scale, Operation="Multiply", StoreInADS=False
        )
        shifted_norm_front = mantid_api.Scale(InputWorkspace=sample_norm_secondary, Factor=shift, Operation="Multiply", StoreInADS=False)
        numerator = mantid_api.Plus(LHSWorkspace=sample_count_secondary, RHSWorkspace=shifted_norm_front, StoreInADS=False)
        hab_sample = mantid_api.Divide(LHSWorkspace=numerator, RHSWorkspace=scaled_norm_front, StoreInADS=False)

        if can_count_secondary is not None and can_norm_secondary is not None:
            scaled_norm_front_can = mantid_api.Scale(
                InputWorkspace=can_norm_secondary, Factor=1.0 / scale, Operation="Multiply", StoreInADS=False
            )
            hab_can = mantid_api.Divide(LHSWorkspace=can_count_secondary, RHSWorkspace=scaled_norm_front_can, StoreInADS=False)
            hab_sample = mantid_api.Minus(LHSWorkspace=hab_sample, RHSWorkspace=hab_can, StoreInADS=False)
            return hab_sample
        else:
            return hab_sample

    def merge(self, reduction_mode_vs_output_bundles, parent_alg=None):
        """
        Merges two partial reductions to obtain a merged reduction.

        :param all_outputs: all ReductionMode vs OutputBundle maps
        :param can_outputs: a ReductionMode vs OutputBundle map for the can workspaces
        :param sample_outputs: a ReductionMode vs OutputBundle map for the sample workspaces
        :param parent_alg: a handle to the parent algorithm.
        :return: a MergeBundle with the merged which contains the merged workspace.
        """
        # Get the primary and secondary detectors for stitching. This is normally LAB and HAB, but in other scenarios
        # there might be completely different detectors. This approach allows future adjustments to the stitching
        # configuration. The data from the secondary detector will be stitched to the data from the primary detector.

        primary_detector, secondary_detector = get_detectors_for_merge(reduction_mode_vs_output_bundles)
        sample_count_primary, sample_norm_primary, sample_count_secondary, sample_norm_secondary = get_partial_workspaces(
            primary_detector, secondary_detector, reduction_mode_vs_output_bundles, is_sample
        )

        # Get the relevant workspaces from the reduction settings. For this we need to first understand what the
        # We can have merged reductions
        can_count_primary, can_norm_primary, can_count_secondary, can_norm_secondary = get_partial_workspaces(
            primary_detector, secondary_detector, reduction_mode_vs_output_bundles, is_can
        )
        # Get fit parameters
        shift_factor, scale_factor, fit_mode, fit_min, fit_max, merge_mask, merge_min, merge_max = get_shift_and_scale_parameter(
            reduction_mode_vs_output_bundles
        )

        fit_mode_as_string = fit_mode.value

        # We need to convert NoFit to None.
        if fit_mode_as_string == "NoFit":
            fit_mode_as_string = "None"

        # Run the SANSStitch algorithm
        stitch_name = "SANSStitch"
        stitch_options = {
            "HABCountsSample": sample_count_secondary,
            "HABNormSample": sample_norm_secondary,
            "LABCountsSample": sample_count_primary,
            "LABNormSample": sample_norm_primary,
            "ProcessCan": False,
            "Mode": fit_mode_as_string,
            "ScaleFactor": scale_factor,
            "ShiftFactor": shift_factor,
            "MergeMask": merge_mask,
            "OutputWorkspace": "dummy",
        }

        if (
            can_count_primary is not None
            and can_norm_primary is not None
            and can_count_secondary is not None
            and can_norm_secondary is not None
        ):
            stitch_options_can = {
                "HABCountsCan": can_count_secondary,
                "HABNormCan": can_norm_secondary,
                "LABCountsCan": can_count_primary,
                "LABNormCan": can_norm_primary,
                "ProcessCan": True,
            }
            stitch_options.update(stitch_options_can)

        if fit_min and fit_max:
            q_range_options = {"FitMin": fit_min, "FitMax": fit_max}
            stitch_options.update(q_range_options)

        if merge_mask:
            if merge_min:
                q_range_options = {"MergeMin": merge_min}
                stitch_options.update(q_range_options)
            if merge_max:
                q_range_options = {"MergeMax": merge_max}
                stitch_options.update(q_range_options)

        stitch_alg = create_child_algorithm(parent_alg, stitch_name, **stitch_options)
        stitch_alg.execute()

        # Get the fit values
        shift_from_alg = stitch_alg.getProperty("OutShiftFactor").value
        scale_from_alg = stitch_alg.getProperty("OutScaleFactor").value
        merged_workspace = stitch_alg.getProperty("OutputWorkspace").value
        scaled_HAB_workspace = self.calculate_scaled_hab_output(
            shift_from_alg, scale_from_alg, sample_count_secondary, sample_norm_secondary, can_count_secondary, can_norm_secondary
        )

        # Return a merge bundle with the merged workspace and the fitted scale and shift factor (they are good
        # diagnostic tools which are desired by the instrument scientists.
        return MergeBundle(
            merged_workspace=merged_workspace, shift=shift_from_alg, scale=scale_from_alg, scaled_hab_workspace=scaled_HAB_workspace
        )


class NullMerger(Merger):
    def __init__(self):
        super(NullMerger, self).__init__()

    def merge(self, reduction_mode_vs_output_bundles, parent_alg=None):
        pass


class MergeFactory(object):
    def __init__(self):
        super(MergeFactory, self).__init__()

    @staticmethod
    def create_merger(state):
        # The selection depends on the facility/instrument
        data_info = state.data
        facility = data_info.facility

        if facility is SANSFacility.ISIS:
            merger = ISIS1DMerger()
        else:
            merger = NullMerger()
            RuntimeError("MergeFactory: The merging for your selection has not been implemented yet.")
        return merger


def get_detectors_for_merge(output_tuples):
    """
    Extracts the merge strategy from the output bundles. This is the name of the primary and the secondary detector.

    The merge strategy will let us know which two detectors are to be merged. This abstraction might be useful in the
    future if we are dealing with more than two detector banks.
    :param output_tuples: paired up reductions
    :return: the primary detector and the secondary detector.
    """
    # Get first element of first list as we don't care about its wav range or if it's a can/sample
    state = next(iter(output_tuples.values()))[0].output_bundle.state
    reduction_info = state.reduction
    return reduction_info.get_merge_strategy()


def get_partial_workspaces(primary_detector, secondary_detector, reduction_mode_vs_output_bundles, is_data_type):
    """
    Get the partial workspaces for the primary and secondary detectors.

    :param primary_detector: the primary detector (now normally ReductionMode.LAB)
    :param secondary_detector: the secondary detector (now normally ReductionMode.HAB)
    :param reduction_mode_vs_output_bundles: a ReductionMode vs OutputBundles map
    :param is_data_type: the data type, i.e. if can or sample
    :return: the primary count workspace, the primary normalization workspace, the secondary count workspace and the
             secondary normalization workspace.
    """
    # Get primary reduction information for specified data type, i.e. sample or can
    primary = reduction_mode_vs_output_bundles[primary_detector]
    primary_for_data_type = next((setting for setting in primary if is_data_type(setting)), None)
    primary_count = primary_for_data_type.parts_bundle.output_workspace_count if primary_for_data_type else None
    primary_norm = primary_for_data_type.parts_bundle.output_workspace_norm if primary_for_data_type else None

    # Get secondary reduction information for specified data type, i.e. sample or can
    secondary = reduction_mode_vs_output_bundles[secondary_detector]
    secondary_for_data_type = next((setting for setting in secondary if is_data_type(setting)), None)
    secondary_count = secondary_for_data_type.parts_bundle.output_workspace_count if secondary_for_data_type else None
    secondary_norm = secondary_for_data_type.parts_bundle.output_workspace_norm if secondary_for_data_type else None
    return primary_count, primary_norm, secondary_count, secondary_norm


def get_shift_and_scale_parameter(reduction_mode_vs_output_bundles):
    """
    Gets the shfit and scale parameter from a set of OutputBundles

    :param reduction_mode_vs_output_bundles: a ReductionMode vs OutputBundle map
    :return: the shift, scale and fit mode.
    """
    reduction_settings_collection = next(iter(list(reduction_mode_vs_output_bundles.values())))
    state = reduction_settings_collection[0].output_bundle.state
    reduction_info = state.reduction

    if reduction_info.merge_range_min:
        fit_min = reduction_info.merge_range_min
    else:
        fit_min = None
    if reduction_info.merge_range_max:
        fit_max = reduction_info.merge_range_max
    else:
        fit_max = None

    return (
        reduction_info.merge_shift,
        reduction_info.merge_scale,
        reduction_info.merge_fit_mode,
        fit_min,
        fit_max,
        reduction_info.merge_mask,
        reduction_info.merge_min,
        reduction_info.merge_max,
    )


def is_sample(x):
    return x.output_bundle.data_type is DataType.SAMPLE


def is_can(x):
    return x.output_bundle.data_type is DataType.CAN
