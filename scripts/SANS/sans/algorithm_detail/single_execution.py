# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
from __future__ import (absolute_import, division, print_function)

import sys

from mantid.kernel import mpisetup
from sans.algorithm_detail.bundles import (EventSliceSettingBundle, OutputBundle,
                                           OutputPartsBundle, OutputTransmissionBundle)
from sans.algorithm_detail.merge_reductions import (MergeFactory, is_sample, is_can)
from sans.algorithm_detail.strip_end_nans_and_infs import strip_end_nans
from sans.common.constants import EMPTY_NAME
from sans.common.enums import (DataType, DetectorType, ISISReductionMode, OutputParts, TransmissionType)
from sans.common.general_functions import (create_child_algorithm, get_reduced_can_workspace_from_ads,
                                           get_transmission_workspaces_from_ads,
                                           write_hash_into_reduced_can_workspace)


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
    serialized_state = reduction_setting_bundle.state.property_manager
    reduction_alg.setProperty("SANSState", serialized_state)
    reduction_alg.setProperty("Component", component)
    reduction_alg.setProperty("ScatterWorkspace", reduction_setting_bundle.scatter_workspace)
    reduction_alg.setProperty("ScatterMonitorWorkspace", reduction_setting_bundle.scatter_monitor_workspace)
    reduction_alg.setProperty("DataType", DataType.to_string(reduction_setting_bundle.data_type))

    reduction_alg.setProperty("OutputWorkspace", EMPTY_NAME)
    reduction_alg.setProperty("OutputMonitorWorkspace", EMPTY_NAME)

    # Run the reduction core
    reduction_alg.execute()

    # Get the results
    output_workspace = reduction_alg.getProperty("OutputWorkspace").value
    mask_workspace = reduction_alg.getProperty("DummyMaskWorkspace").value
    output_monitor_workspace = reduction_alg.getProperty("OutputMonitorWorkspace").value

    return EventSliceSettingBundle(state=reduction_setting_bundle.state,
                                   data_type=reduction_setting_bundle.data_type,
                                   reduction_mode=reduction_setting_bundle.reduction_mode,
                                   output_parts=reduction_setting_bundle.output_parts,
                                   scatter_workspace=output_workspace,
                                   dummy_mask_workspace=mask_workspace,
                                   scatter_monitor_workspace=output_monitor_workspace,
                                   direct_workspace=reduction_setting_bundle.direct_workspace,
                                   transmission_workspace=reduction_setting_bundle.transmission_workspace)


def run_core_event_slice_reduction(reduction_alg, reduction_setting_bundle):
    """
    This function runs a core reduction for event slice data. This reduction slices by event time and converts to q.
    All other operations, such as moving and converting to histogram, have been performed before the event slicing.

    :param reduction_alg: a handle to the reduction algorithm.
    :param reduction_setting_bundle: a ReductionSettingBundle tuple
    :return: an OutputBundle and an OutputPartsBundle
    """

    # Get component to reduce
    component = get_component_to_reduce(reduction_setting_bundle)
    # Set the properties on the reduction algorithms
    serialized_state = reduction_setting_bundle.state.property_manager
    reduction_alg.setProperty("SANSState", serialized_state)
    reduction_alg.setProperty("Component", component)
    reduction_alg.setProperty("ScatterWorkspace", reduction_setting_bundle.scatter_workspace)
    reduction_alg.setProperty("DirectWorkspace", reduction_setting_bundle.direct_workspace)
    reduction_alg.setProperty("TransmissionWorkspace", reduction_setting_bundle.transmission_workspace)
    reduction_alg.setProperty("DummyMaskWorkspace", reduction_setting_bundle.dummy_mask_workspace)
    reduction_alg.setProperty("ScatterMonitorWorkspace", reduction_setting_bundle.scatter_monitor_workspace)

    reduction_alg.setProperty("DataType", DataType.to_string(reduction_setting_bundle.data_type))

    reduction_alg.setProperty("OutputWorkspace", EMPTY_NAME)
    reduction_alg.setProperty("SumOfCounts", EMPTY_NAME)
    reduction_alg.setProperty("SumOfNormFactors", EMPTY_NAME)

    # Run the reduction core
    reduction_alg.execute()

    # Get the results
    output_workspace = reduction_alg.getProperty("OutputWorkspace").value
    output_workspace_count = reduction_alg.getProperty("SumOfCounts").value
    output_workspace_norm = reduction_alg.getProperty("SumOfNormFactors").value
    output_calculated_transmission_workspace = reduction_alg.getProperty("CalculatedTransmissionWorkspace").value
    output_unfitted_transmission_workspace = reduction_alg.getProperty("UnfittedTransmissionWorkspace").value

    # Pull the result out of the workspace
    output_bundle = OutputBundle(state=reduction_setting_bundle.state,
                                 data_type=reduction_setting_bundle.data_type,
                                 reduction_mode=reduction_setting_bundle.reduction_mode,
                                 output_workspace=output_workspace)

    output_parts_bundle = OutputPartsBundle(state=reduction_setting_bundle.state,
                                            data_type=reduction_setting_bundle.data_type,
                                            reduction_mode=reduction_setting_bundle.reduction_mode,
                                            output_workspace_count=output_workspace_count,
                                            output_workspace_norm=output_workspace_norm)

    output_transmission_bundle = OutputTransmissionBundle(state=reduction_setting_bundle.state,
                                                          data_type=reduction_setting_bundle.data_type,
                                                          calculated_transmission_workspace=
                                                          output_calculated_transmission_workspace,
                                                          unfitted_transmission_workspace=
                                                          output_unfitted_transmission_workspace,
                                                          )
    return output_bundle, output_parts_bundle, output_transmission_bundle


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
    serialized_state = reduction_setting_bundle.state.property_manager
    reduction_alg.setProperty("SANSState", serialized_state)
    reduction_alg.setProperty("Component", component)
    reduction_alg.setProperty("ScatterWorkspace", reduction_setting_bundle.scatter_workspace)
    reduction_alg.setProperty("ScatterMonitorWorkspace", reduction_setting_bundle.scatter_monitor_workspace)
    reduction_alg.setProperty("DataType", DataType.to_string(reduction_setting_bundle.data_type))

    if reduction_setting_bundle.transmission_workspace is not None:
        reduction_alg.setProperty("TransmissionWorkspace", reduction_setting_bundle.transmission_workspace)

    if reduction_setting_bundle.direct_workspace is not None:
        reduction_alg.setProperty("DirectWorkspace", reduction_setting_bundle.direct_workspace)

    reduction_alg.setProperty("OutputWorkspace", EMPTY_NAME)
    reduction_alg.setProperty("SumOfCounts", EMPTY_NAME)
    reduction_alg.setProperty("SumOfNormFactors", EMPTY_NAME)

    # Run the reduction core
    reduction_alg.execute()

    # Get the results
    output_workspace = reduction_alg.getProperty("OutputWorkspace").value
    output_workspace_count = reduction_alg.getProperty("SumOfCounts").value
    output_workspace_norm = reduction_alg.getProperty("SumOfNormFactors").value
    output_calculated_transmission_workspace = reduction_alg.getProperty("CalculatedTransmissionWorkspace").value
    output_unfitted_transmission_workspace = reduction_alg.getProperty("UnfittedTransmissionWorkspace").value
    # Pull the result out of the workspace
    output_bundle = OutputBundle(state=reduction_setting_bundle.state,
                                 data_type=reduction_setting_bundle.data_type,
                                 reduction_mode=reduction_setting_bundle.reduction_mode,
                                 output_workspace=output_workspace)

    output_parts_bundle = OutputPartsBundle(state=reduction_setting_bundle.state,
                                            data_type=reduction_setting_bundle.data_type,
                                            reduction_mode=reduction_setting_bundle.reduction_mode,
                                            output_workspace_count=output_workspace_count,
                                            output_workspace_norm=output_workspace_norm)

    output_transmission_bundle = OutputTransmissionBundle(state=reduction_setting_bundle.state,
                                                          data_type=reduction_setting_bundle.data_type,
                                                          calculated_transmission_workspace=output_calculated_transmission_workspace,
                                                          unfitted_transmission_workspace=output_unfitted_transmission_workspace
                                                          )
    return output_bundle, output_parts_bundle, output_transmission_bundle


def get_final_output_workspaces(output_bundles, parent_alg):
    """
    This function provides the final steps for the data reduction.

    The final steps are:
    1. Can Subtraction (if required)
    2. Data clean up (if required)
    :param output_bundles: A set of outputBundles
    :param parent_alg: a handle to the parent algorithm.
    :return: a map of ReductionMode vs final output workspaces.
    """

    reduction_mode_vs_output_bundles = get_reduction_mode_vs_output_bundles(output_bundles)

    # For each reduction mode, we need to perform a can subtraction (and potential cleaning of the workspace)
    final_output_workspaces = {}
    for reduction_mode, output_bundles in reduction_mode_vs_output_bundles.items():
        # Find the sample and the can in the data collection
        output_sample_workspace = next((output_bundle.output_workspace for output_bundle in output_bundles
                                        if is_sample(output_bundle)), None)
        output_can_workspace = next((output_bundle.output_workspace for output_bundle in output_bundles
                                     if is_can(output_bundle)), None)
        # Perform the can subtraction
        if output_can_workspace is not None:
            final_output_workspace = perform_can_subtraction(output_sample_workspace, output_can_workspace, parent_alg)
        else:
            final_output_workspace = output_sample_workspace

        # Tidy up the workspace by removing start/end-NANs and start/end-INFs
        final_output_workspace = strip_end_nans(final_output_workspace, parent_alg)
        final_output_workspaces.update({reduction_mode: final_output_workspace})

    # Finally add sample log information
    # TODO: Add log information

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
    subtraction_options = {"LHSWorkspace": sample,
                           "RHSWorkspace": can,
                           "OutputWorkspace": EMPTY_NAME}
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
    _ = can_workspace  # noqa
    if sample_workspace.getNumberHistograms() == 1 and sample_workspace.hasDx(0):
        subtracted_workspace.setDx(0, sample_workspace.dataDx(0))


def get_merge_bundle_for_merge_request(output_bundles, parent_alg):
    """
    Create a merge bundle for the reduction outputs and perform stitching if required
    :param output_bundles: a list of output_bundles
    :param parent_alg: a handle to the parent algorithm
    """
    # Order the reductions. This leaves us with a dict mapping from the reduction type (i.e. HAB, LAB) to
    # a list of reduction settings which contain the information for sample and can.
    reduction_mode_vs_output_bundles = get_reduction_mode_vs_output_bundles(output_bundles)

    # Get the underlying state from one of the elements
    state = output_bundles[0].state

    merge_factory = MergeFactory()
    merger = merge_factory.create_merger(state)

    # Run the merger and return the merged output workspace
    return merger.merge(reduction_mode_vs_output_bundles, parent_alg)


def get_reduction_mode_vs_output_bundles(output_bundles):
    """
    Groups the reduction information by the reduction mode, e.g. all information regarding HAB is collated, similarly
    for LAB.
    """
    outputs = {}
    # Pair up the different reduction modes
    for output_bundle in output_bundles:
        key = output_bundle.reduction_mode
        if key in outputs:
            outputs[key].append(output_bundle)
        else:
            outputs.update({key: [output_bundle]})
    return outputs


def get_component_to_reduce(reduction_setting_bundle):
    """
    Gets the component to reduce as string. Currently we encode this as LAB or HAB.

    :param reduction_setting_bundle: a ReductionSettingBundle tuple.
    :return: the reduction mode as a string.
    """
    # Get the reduction mode
    reduction_mode = reduction_setting_bundle.reduction_mode

    if reduction_mode is ISISReductionMode.LAB:
        reduction_mode_setting = DetectorType.to_string(DetectorType.LAB)
    elif reduction_mode is ISISReductionMode.HAB:
        reduction_mode_setting = DetectorType.to_string(DetectorType.HAB)
    else:
        raise RuntimeError("SingleExecution: An unknown reduction mode was selected: {}. "
                           "Currently only HAB and LAB are supported.".format(reduction_mode))
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
    data_type = reduction_setting_bundle.data_type
    reduced_can_workspace, reduced_can_workspace_count, reduced_can_workspace_norm = \
        get_reduced_can_workspace_from_ads(state, output_parts, reduction_mode)
    output_calculated_transmission_workspace, output_unfitted_transmission_workspace = \
        get_transmission_workspaces_from_ads(state, reduction_mode)
    # Set the results on the output bundle
    output_bundle = OutputBundle(state=state, data_type=data_type, reduction_mode=reduction_mode,
                                 output_workspace=reduced_can_workspace)
    output_parts_bundle = OutputPartsBundle(state=state, data_type=data_type, reduction_mode=reduction_mode,
                                            output_workspace_count=reduced_can_workspace_count,
                                            output_workspace_norm=reduced_can_workspace_norm)
    output_transmission_bundle = OutputTransmissionBundle(state=reduction_setting_bundle.state, data_type=data_type,
                                                          calculated_transmission_workspace=output_calculated_transmission_workspace,
                                                          unfitted_transmission_workspace=output_unfitted_transmission_workspace
                                                          )
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

    is_invalid_partial_workspaces = ((output_parts_bundle.output_workspace_count is None and
                                      output_parts_bundle.output_workspace_norm is not None) or
                                     (output_parts_bundle.output_workspace_count is not None and
                                      output_parts_bundle.output_workspace_norm is None))
    is_invalid_transmission_workspaces = (output_transmission_bundle.calculated_transmission_workspace is None
                                          or output_transmission_bundle.unfitted_transmission_workspace is None)
    partial_output_require_reload = output_parts and is_invalid_partial_workspaces

    must_reload = output_bundle.output_workspace is None or partial_output_require_reload or is_invalid_transmission_workspaces
    if 'boost.mpi' in sys.modules:
        # In MPI runs the result is only present on rank 0 (result of Q1D2 integration),
        # so the reload flag must be broadcasted from rank 0.
        must_reload = mpisetup.boost.mpi.broadcast(mpisetup.boost.mpi.world, must_reload, 0)

    if must_reload:
        # if output_bundle.output_workspace is None or partial_output_require_reload:
        if not event_slice_optimisation:
            output_bundle, output_parts_bundle, \
                output_transmission_bundle = run_core_reduction(reduction_alg, reduction_setting_bundle)
        else:
            output_bundle, output_parts_bundle, \
                output_transmission_bundle = run_core_event_slice_reduction(reduction_alg, reduction_setting_bundle)

        # Now we need to tag the workspaces and add it to the ADS
        if output_bundle.output_workspace is not None:
            write_hash_into_reduced_can_workspace(state=output_bundle.state,
                                                  workspace=output_bundle.output_workspace,
                                                  partial_type=None,
                                                  reduction_mode=reduction_mode)
        if output_transmission_bundle.calculated_transmission_workspace is not None and \
                output_transmission_bundle.unfitted_transmission_workspace is not None:
            write_hash_into_reduced_can_workspace(state=output_transmission_bundle.state,
                                                  workspace=output_transmission_bundle.calculated_transmission_workspace,
                                                  partial_type=TransmissionType.Calculated,
                                                  reduction_mode=reduction_mode)
            write_hash_into_reduced_can_workspace(state=output_transmission_bundle.state,
                                                  workspace=output_transmission_bundle.unfitted_transmission_workspace,
                                                  partial_type=TransmissionType.Unfitted,
                                                  reduction_mode=reduction_mode)
        if (output_parts_bundle.output_workspace_count is not None and
                output_parts_bundle.output_workspace_norm is not None):
            write_hash_into_reduced_can_workspace(state=output_parts_bundle.state,
                                                  workspace=output_parts_bundle.output_workspace_count,
                                                  partial_type=OutputParts.Count,
                                                  reduction_mode=reduction_mode)

            write_hash_into_reduced_can_workspace(state=output_parts_bundle.state,
                                                  workspace=output_parts_bundle.output_workspace_norm,
                                                  partial_type=OutputParts.Norm,
                                                  reduction_mode=reduction_mode)

    return output_bundle, output_parts_bundle, output_transmission_bundle
