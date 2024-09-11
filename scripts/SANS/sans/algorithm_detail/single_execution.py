# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import os
from collections import OrderedDict
from typing import List, Tuple

from mantid.kernel import logger
from mantid.dataobjects import Workspace2D
from sans.algorithm_detail.bundles import (
    EventSliceSettingBundle,
    OutputBundle,
    OutputPartsBundle,
    OutputTransmissionBundle,
    CompletedSlices,
    ReducedSlice,
)
from sans.algorithm_detail.merge_reductions import MergeFactory
from sans.algorithm_detail.strip_end_nans_and_infs import strip_end_nans
from sans.common.constants import EMPTY_NAME
from sans.common.enums import DetectorType, ReductionMode, OutputParts, TransmissionType, DataType
from sans.common.general_functions import (
    create_child_algorithm,
    get_reduced_can_workspace_from_ads,
    get_transmission_workspaces_from_ads,
    write_hash_into_reduced_can_workspace,
    wav_range_to_str,
)
from sans.data_objects.sans_workflow_algorithm_outputs import SANSWorkflowAlgorithmOutputs
from sans.state.Serializer import Serializer


def run_initial_event_slice_reduction(reduction_alg, reduction_setting_bundle):
    """
    This function runs the initial core reduction for event slice data. This is essentially half
    a reduction (either sample or can), and is run before event slicing has been performed.

    :param reduction_alg: a handle to the initial event slice reduction algorithm.
    :param reduction_setting_bundle: a ReductionSettingBundle tuple
    :return: a EventSliceReductionSettingBundle tuple
    """
    # Get component to reduce
    component = get_component_to_reduce(reduction_setting_bundle)
    # Set the properties on the reduction algorithms
    serialized_state = Serializer.to_json(reduction_setting_bundle.state)
    reduction_alg.setProperty("SANSState", serialized_state)
    reduction_alg.setProperty("Component", component)
    reduction_alg.setProperty("ScatterWorkspace", reduction_setting_bundle.scatter_workspace)
    reduction_alg.setProperty("ScatterMonitorWorkspace", reduction_setting_bundle.scatter_monitor_workspace)
    reduction_alg.setProperty("DataType", reduction_setting_bundle.data_type.value)

    reduction_alg.setProperty("OutputWorkspace", EMPTY_NAME)
    reduction_alg.setProperty("OutputMonitorWorkspace", EMPTY_NAME)

    # Run the reduction core
    reduction_alg.execute()

    # Get the results
    output_workspace = reduction_alg.getProperty("OutputWorkspace").value
    mask_workspace = reduction_alg.getProperty("DummyMaskWorkspace").value
    output_monitor_workspace = reduction_alg.getProperty("OutputMonitorWorkspace").value

    return EventSliceSettingBundle(
        state=reduction_setting_bundle.state,
        data_type=reduction_setting_bundle.data_type,
        reduction_mode=reduction_setting_bundle.reduction_mode,
        output_parts=reduction_setting_bundle.output_parts,
        scatter_workspace=output_workspace,
        dummy_mask_workspace=mask_workspace,
        scatter_monitor_workspace=output_monitor_workspace,
        direct_workspace=reduction_setting_bundle.direct_workspace,
        transmission_workspace=reduction_setting_bundle.transmission_workspace,
    )


def run_core_event_slice_reduction(reduction_alg, reduction_setting_bundle):
    """
    This function runs a core reduction for event slice data. This reduction slices by event time and converts to q.
    All other operations, such as moving and converting to histogram, have been performed before the event slicing.

    :param reduction_alg: a handle to the reduction algorithm.
    :param reduction_setting_bundle: a ReductionSettingBundle tuple
    :return: a list of reduced slices
    """

    # Get component to reduce
    component = get_component_to_reduce(reduction_setting_bundle)
    # Set the properties on the reduction algorithms
    serialized_state = Serializer.to_json(reduction_setting_bundle.state)
    reduction_alg.setProperty("SANSState", serialized_state)
    reduction_alg.setProperty("Component", component)
    reduction_alg.setProperty("ScatterWorkspace", reduction_setting_bundle.scatter_workspace)
    reduction_alg.setProperty("DirectWorkspace", reduction_setting_bundle.direct_workspace)
    reduction_alg.setProperty("TransmissionWorkspace", reduction_setting_bundle.transmission_workspace)
    reduction_alg.setProperty("DummyMaskWorkspace", reduction_setting_bundle.dummy_mask_workspace)
    reduction_alg.setProperty("ScatterMonitorWorkspace", reduction_setting_bundle.scatter_monitor_workspace)

    reduction_alg.setProperty("DataType", reduction_setting_bundle.data_type.value)

    reduction_alg.setProperty("OutputWorkspaces", EMPTY_NAME)
    reduction_alg.setProperty("SumOfCounts", EMPTY_NAME)
    reduction_alg.setProperty("SumOfNormFactors", EMPTY_NAME)

    # Run the reduction core
    reduction_alg.execute()

    # Pull the result out of the workspace
    reduced_slices = _pack_bundles(reduction_alg, reduction_setting_bundle)

    return reduced_slices


def run_core_reduction(reduction_alg, reduction_setting_bundle):
    """
    This function runs a core reduction. This is essentially half a reduction (either sample or can).

    :param reduction_alg: a handle to the reduction algorithm.
    :param reduction_setting_bundle: a ReductionSettingBundle tuple
    :return: an OutputBundle and an OutputPartsBundle
    """

    # Get component to reduce
    component = get_component_to_reduce(reduction_setting_bundle)
    # Set the properties on the reduction algorithms
    serialized_state = Serializer.to_json(reduction_setting_bundle.state)
    reduction_alg.setProperty("SANSState", serialized_state)
    reduction_alg.setProperty("Component", component)
    reduction_alg.setProperty("ScatterWorkspace", reduction_setting_bundle.scatter_workspace)
    reduction_alg.setProperty("ScatterMonitorWorkspace", reduction_setting_bundle.scatter_monitor_workspace)
    reduction_alg.setProperty("DataType", reduction_setting_bundle.data_type.value)

    if reduction_setting_bundle.transmission_workspace is not None:
        reduction_alg.setProperty("TransmissionWorkspace", reduction_setting_bundle.transmission_workspace)

    if reduction_setting_bundle.direct_workspace is not None:
        reduction_alg.setProperty("DirectWorkspace", reduction_setting_bundle.direct_workspace)

    reduction_alg.setProperty("OutputWorkspaces", EMPTY_NAME)
    reduction_alg.setProperty("SumOfCounts", EMPTY_NAME)
    reduction_alg.setProperty("SumOfNormFactors", EMPTY_NAME)

    # Run the reduction core
    reduction_alg.execute()

    # Get the results
    reduced_slices = _pack_bundles(reduction_alg, reduction_setting_bundle)
    return reduced_slices


def _pack_bundles(reduction_alg, reduction_setting_bundle):
    output_workspaces = reduction_alg.getProperty("OutputWorkspaces").value
    output_workspace_counts = reduction_alg.getProperty("SumOfCounts").value
    output_workspace_norms = reduction_alg.getProperty("SumOfNormFactors").value
    out_trans_ws = reduction_alg.getProperty("CalculatedTransmissionWorkspaces").value
    out_unfit_trans_ws = reduction_alg.getProperty("UnfittedTransmissionWorkspaces").value

    slices: CompletedSlices = []
    for i, reduced_ws in enumerate(output_workspaces):
        calc_trans_ws = out_trans_ws.getItem(i) if out_trans_ws else None
        unfit_trans_ws = out_unfit_trans_ws.getItem(i) if out_unfit_trans_ws else None

        slice = ReducedSlice(
            wav_range=reduced_ws.getRun().getProperty("Wavelength Range").valueAsStr,
            output_bundle=OutputBundle(
                state=reduction_setting_bundle.state,
                data_type=reduction_setting_bundle.data_type,
                reduction_mode=reduction_setting_bundle.reduction_mode,
                output_workspace=reduced_ws,
            ),
            parts_bundle=OutputPartsBundle(
                state=reduction_setting_bundle.state,
                data_type=reduction_setting_bundle.data_type,
                reduction_mode=reduction_setting_bundle.reduction_mode,
                output_workspace_count=output_workspace_counts.getItem(i),
                output_workspace_norm=output_workspace_norms.getItem(i),
            ),
            transmission_bundle=OutputTransmissionBundle(
                state=reduction_setting_bundle.state,
                data_type=reduction_setting_bundle.data_type,
                calculated_transmission_workspace=calc_trans_ws,
                unfitted_transmission_workspace=unfit_trans_ws,
            ),
        )
        slices.append(slice)
    return slices


def pair_up_wav_ranges(list_to_pair: CompletedSlices) -> List[Tuple[ReducedSlice]]:
    # We need to preserve order for system tests, so we can't use set directly
    unique_wav_lengths = list(OrderedDict.fromkeys(map(lambda i: i.wav_range, list_to_pair)))
    packed = [tuple(filter(lambda x: x.wav_range == k, list_to_pair)) for k in unique_wav_lengths]
    # If we have >2 elements we need to consider another attribute as they aren't unique enough
    assert all(len(i) <= 2 for i in packed)
    return packed


def get_final_output_workspaces(completed_event_slices, parent_alg) -> SANSWorkflowAlgorithmOutputs:
    """
    This function provides the final steps for the data reduction.

    The final steps are:
    1. Can Subtraction (if required)
    2. Data clean up (if required)
    :param completed_event_slices: A list of completed output bundles
    :param parent_alg: a handle to the parent algorithm.
    :return: a map of ReductionMode vs final output workspaces.
    """
    hab_lab_outputs = get_reduction_mode_vs_output_bundles(completed_event_slices)
    for k, v in hab_lab_outputs.items():
        hab_lab_outputs[k] = pair_up_wav_ranges(v)

    # For each reduction mode, we need to perform a can subtraction (and potential cleaning of the workspace)
    lab_workspaces = _pack_outputs(hab_lab_outputs.pop(ReductionMode.LAB, None), parent_alg)
    hab_workspaces = _pack_outputs(hab_lab_outputs.pop(ReductionMode.HAB, None), parent_alg)

    return SANSWorkflowAlgorithmOutputs(lab_output=lab_workspaces, hab_output=hab_workspaces)


def _pack_outputs(reductions, parent_alg) -> List[Workspace2D]:
    if not reductions:
        return []
    final_output_workspaces = []
    for paired_reductions in reductions:
        can = next(filter(lambda i: i.output_bundle.data_type is DataType.CAN, paired_reductions), None)
        sample = next(filter(lambda i: i.output_bundle.data_type is DataType.SAMPLE, paired_reductions))
        # Perform the can subtraction
        if can:
            final_output_workspace = perform_can_subtraction(
                sample.output_bundle.output_workspace, can.output_bundle.output_workspace, parent_alg
            )
        else:
            final_output_workspace = sample.output_bundle.output_workspace

        # Tidy up the workspace by removing start/end-NANs and start/end-INFs
        final_output_workspace = strip_end_nans(final_output_workspace, parent_alg)
        final_output_workspaces.append(final_output_workspace)
    return final_output_workspaces


def perform_can_subtraction(sample, can, parent_alg):
    """
    Subtracts the can from the sample workspace.

    We need to manually take care of the q resolution issue here.
    :param sample: the sample workspace
    :param can: the can workspace.
    :param parent_alg: a handle to the parent algorithm.
    :return: the subtracted workspace.
    """
    subtraction_name = "Minus"
    subtraction_options = {"LHSWorkspace": sample, "RHSWorkspace": can, "OutputWorkspace": EMPTY_NAME}
    subtraction_alg = create_child_algorithm(parent_alg, subtraction_name, **subtraction_options)
    subtraction_alg.execute()
    output_workspace = subtraction_alg.getProperty("OutputWorkspace").value

    # If the workspace is 1D and contains Q resolution (i.e. DX values), then we need to make sure that the
    # resulting output workspace contains the correct values
    correct_q_resolution_for_can(sample, can, output_workspace)

    return output_workspace


def correct_q_resolution_for_can(sample_workspace, can_workspace, subtracted_workspace):
    """
    Sets the correct Q resolution on a can-subtracted workspace.

    We need to transfer the Q resolution from the original workspaces to the subtracted
    workspace. Richard wants us to ignore potential DX values for the CAN workspace (they
    would be very small any way). The Q resolution functionality only exists currently
    for 1D, ie when only one spectrum is present.
    """
    _ = can_workspace
    if sample_workspace.getNumberHistograms() == 1 and sample_workspace.hasDx(0):
        subtracted_workspace.setDx(0, sample_workspace.dataDx(0))


def get_merge_bundle_for_merge_request(completed_slices: CompletedSlices, parent_alg):
    """
    Create a merge bundle for the reduction outputs and perform stitching if required
    :param completed_slices: a list of output_bundles
    :param parent_alg: a handle to the parent algorithm
    """
    # Order the reductions. This leaves us with a dict mapping from the reduction type (i.e. HAB, LAB) to
    # a list of reduction settings which contain the information for sample and can.
    wav_ranges = {k.wav_range for k in completed_slices}
    bank_based_reductions = []
    for wav_range in wav_ranges:
        matching_reductions = [i for i in completed_slices if i.wav_range == wav_range]
        bank_based_reductions.append(get_reduction_mode_vs_output_bundles(matching_reductions))

    # Get the underlying state from one of the elements
    state = completed_slices[0].output_bundle.state

    merge_factory = MergeFactory()
    merger = merge_factory.create_merger(state)

    # Run the merger and return the merged output workspace
    merged_workspaces = []
    for matched_reductions in bank_based_reductions:
        merged = merger.merge(matched_reductions, parent_alg)
        replace_prop = True
        if state.save.user_file:
            merged.merged_workspace.getRun().addProperty("UserFile", os.path.basename(state.save.user_file), replace_prop)
        if state.save.batch_file:
            merged.merged_workspace.getRun().addProperty("BatchFile", os.path.basename(state.save.batch_file), replace_prop)
        merged_workspaces.append(merged)
    return merged_workspaces


def get_reduction_mode_vs_output_bundles(completed_event_slice_bundles):
    """
    Groups the reduction information by the reduction mode, e.g. all information regarding HAB is collated, similarly
    for LAB.
    """
    outputs = {}

    def pack_reduction_modes(bundle: ReducedSlice):
        key = bundle.output_bundle.reduction_mode
        if key in outputs:
            outputs[key].append(bundle)
        else:
            outputs.update({key: [bundle]})

    # Pair up the different reduction modes
    for event_slice in completed_event_slice_bundles:
        pack_reduction_modes(event_slice)

    return outputs


def get_component_to_reduce(reduction_setting_bundle):
    """
    Gets the component to reduce as string. Currently we encode this as LAB or HAB.

    :param reduction_setting_bundle: a ReductionSettingBundle tuple.
    :return: the reduction mode as a string.
    """
    # Get the reduction mode
    reduction_mode = reduction_setting_bundle.reduction_mode

    if reduction_mode is ReductionMode.LAB:
        reduction_mode_setting = DetectorType.LAB.value
    elif reduction_mode is ReductionMode.HAB:
        reduction_mode_setting = DetectorType.HAB.value
    else:
        raise RuntimeError(
            "SingleExecution: An unknown reduction mode was selected: {}. Currently only HAB and LAB are supported.".format(reduction_mode)
        )
    return reduction_mode_setting


def run_optimized_for_can(reduction_alg, reduction_setting_bundle, event_slice_optimisation=False):
    """
    Check if the state can reduction already exists, and if so, use it else reduce it and add it to the ADS.

    :param reduction_alg: a handle to the SANSReductionCore algorithm
    :param reduction_setting_bundle: a ReductionSettingBundle tuple.
    :param event_slice_optimisation: An optional bool. If true then run run_core_event_slice_reduction, else run_core_reduction.
    :return: a reduced workspace, a partial output workspace for the counts, a partial workspace for the normalization.
    """
    state = reduction_setting_bundle.state
    output_parts = reduction_setting_bundle.output_parts
    reduction_mode = reduction_setting_bundle.reduction_mode
    reduced_slices = _get_existing_cans(reduction_setting_bundle, state)

    # The logic table for the recalculation of the partial outputs is:
    # | output_parts | reduced_can_workspace_count is None |  reduced_can_workspace_norm is None | Recalculate |
    # ----------------------------------------------------------------------------------------------------------
    # |  False       |        True                         |           True                      |    False    |
    # |  False       |        True                         |           False                     |    False    |
    # |  False       |        False                        |           True                      |    False    |
    # |  False       |        False                        |           False                     |    False    |
    # |  True        |        True                         |           True                      |    False    |
    # |  True        |        True                         |           False                     |    True     |
    # |  True        |        False                        |           True                      |    True     |
    # |  True        |        False                        |           False                     |    False    |

    def needs_reload(slice: ReducedSlice):
        output_parts_bundle = slice.parts_bundle

        # Invalid partial when one is set to none, but not the other
        is_invalid_partial_workspaces = (output_parts_bundle.output_workspace_count is None) ^ (
            output_parts_bundle.output_workspace_norm is None
        )

        output_transmission_bundle = slice.transmission_bundle
        is_invalid_transmission_workspaces = (
            output_transmission_bundle.calculated_transmission_workspace is None
            or output_transmission_bundle.unfitted_transmission_workspace is None
        )
        partial_output_require_reload = output_parts and is_invalid_partial_workspaces

        return slice.output_bundle.output_workspace is None or partial_output_require_reload or is_invalid_transmission_workspaces

    must_reload = any(needs_reload(i) for i in reduced_slices)  # If any slice requires a reload

    if not must_reload:
        # Return the cached can without doing anything else
        logger.information("SANS single_execution: Using cached can workspaces")
        return reduced_slices

    logger.information("SANS single_execution: Processing new/changed can workspaces")
    # We can't used a cached can, lets re-process
    if not event_slice_optimisation:
        reduced_slices = run_core_reduction(reduction_alg, reduction_setting_bundle)
    else:
        reduced_slices = run_core_event_slice_reduction(reduction_alg, reduction_setting_bundle)

    # Now we need to tag the workspaces and add it to the ADS
    for completed_slice in reduced_slices:
        out_bundle = completed_slice.output_bundle
        wav_range = completed_slice.wav_range
        write_hash_into_reduced_can_workspace(
            state=out_bundle.state,
            workspace=out_bundle.output_workspace,
            partial_type=None,
            wav_range=wav_range,
            reduction_mode=reduction_mode,
        )
        trans_bundle = completed_slice.transmission_bundle
        if trans_bundle.calculated_transmission_workspace and trans_bundle.unfitted_transmission_workspace:
            write_hash_into_reduced_can_workspace(
                state=trans_bundle.state,
                workspace=trans_bundle.calculated_transmission_workspace,
                partial_type=TransmissionType.CALCULATED,
                reduction_mode=reduction_mode,
                wav_range=wav_range,
            )
            write_hash_into_reduced_can_workspace(
                state=trans_bundle.state,
                workspace=trans_bundle.unfitted_transmission_workspace,
                partial_type=TransmissionType.UNFITTED,
                reduction_mode=reduction_mode,
                wav_range=None,
            )
        parts_bundle = completed_slice.parts_bundle
        if parts_bundle.output_workspace_count and parts_bundle.output_workspace_norm:
            write_hash_into_reduced_can_workspace(
                state=parts_bundle.state,
                workspace=parts_bundle.output_workspace_count,
                partial_type=OutputParts.COUNT,
                reduction_mode=reduction_mode,
                wav_range=wav_range,
            )

            write_hash_into_reduced_can_workspace(
                state=parts_bundle.state,
                workspace=parts_bundle.output_workspace_norm,
                partial_type=OutputParts.NORM,
                reduction_mode=reduction_mode,
                wav_range=wav_range,
            )

    return reduced_slices


def _get_existing_cans(reduction_setting_bundle, state):
    output_parts = reduction_setting_bundle.output_parts
    reduction_mode = reduction_setting_bundle.reduction_mode
    data_type = reduction_setting_bundle.data_type

    existing_slices = []
    for wav_slice in state.wavelength.wavelength_interval.selected_ranges:
        wav_string = wav_range_to_str(wav_slice)

        reduced_can_workspace, reduced_can_workspace_count, reduced_can_workspace_norm = get_reduced_can_workspace_from_ads(
            state, output_parts, reduction_mode, wav_string
        )
        output_calculated_transmission_workspace, output_unfitted_transmission_workspace = get_transmission_workspaces_from_ads(
            state, reduction_mode, wav_string
        )
        # Set the results on the output bundle
        existing_slices.append(
            ReducedSlice(
                wav_range=wav_string,
                output_bundle=OutputBundle(
                    state=state, data_type=data_type, reduction_mode=reduction_mode, output_workspace=reduced_can_workspace
                ),
                parts_bundle=OutputPartsBundle(
                    state=state,
                    data_type=data_type,
                    reduction_mode=reduction_mode,
                    output_workspace_count=reduced_can_workspace_count,
                    output_workspace_norm=reduced_can_workspace_norm,
                ),
                transmission_bundle=OutputTransmissionBundle(
                    state=reduction_setting_bundle.state,
                    data_type=data_type,
                    calculated_transmission_workspace=output_calculated_transmission_workspace,
                    unfitted_transmission_workspace=output_unfitted_transmission_workspace,
                ),
            )
        )
    return existing_slices
