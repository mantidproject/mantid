from SANS2.Common.SANSConstants import SANSConstants
from SANS2.Common.SANSFunctions import create_unmanaged_algorithm
from SANS.Single.StripEndNansAndInfs import strip_end_nans
from SANS.Single.MergeReductions import (MergeFactory, is_sample, is_can)
from SANS.Single.Bundles import (OutputBundle, OutputPartsBundle)
from SANS2.Common.SANSEnumerations import (ISISReductionMode)


def run_core_reduction(reduction_alg, reduction_setting_bundle, use_optimizations):
    """
    This function runs a core reduction. This is essentially half a reduction (either smaple or can).

    :param reduction_alg: a handle to the reduction algorithm.
    :param reduction_setting_bundle: a ReductionSettingBundle tuple
    :param use_optimizations: a flag to check if optimizations should be used.
    :return: an OutputBundle and an OutputPartsBundle
    """
    # Get component to reduce
    component = get_component_to_reduce(reduction_setting_bundle)
    # Set the properties on the reduction algorithms
    serialized_state = reduction_setting_bundle.state.property_manager
    reduction_alg.setProperty("SANSState", serialized_state)
    reduction_alg.setProperty("UseOptimizations", use_optimizations)
    reduction_alg.setProperty("Component", component)
    reduction_alg.setProperty("ScatterWorkspace", reduction_setting_bundle.scatter_workspace)
    reduction_alg.setProperty("ScatterMonitorWorkspace", reduction_setting_bundle.scatter_monitor_workspace)

    if reduction_setting_bundle.transmission_workspace is not None:
        reduction_alg.setProperty("TransmissionWorkspace", reduction_setting_bundle.transmission_workspace)

    if reduction_setting_bundle.direct_workspace is not None:
        reduction_alg.setProperty("DirectWorkspace", reduction_setting_bundle.direct_workspace)

    reduction_alg.setProperty(SANSConstants.output_workspace, SANSConstants.dummy)
    reduction_alg.setProperty("SumOfCounts", "dummy2")
    reduction_alg.setProperty("SumOfNormFactors", "dummy3")

    # Run the reduction core
    reduction_alg.execute()

    # Get the results
    output_workspace = reduction_alg.getProperty(SANSConstants.output_workspace).value
    output_workspace_count = reduction_alg.getProperty("SumOfCounts").value
    output_workspace_norm = reduction_alg.getProperty("SumOfNormFactors").value

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
    return output_bundle, output_parts_bundle


def get_final_output_workspaces(output_bundles):
    """
    This function provides the final steps for the data reduction.

    The final steps are:
    1. Can Subtraction (if required)
    2. Data clean up (if required)
    :param output_bundles: A set of outputBundles
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
            final_output_workspace = perform_can_subtraction(output_sample_workspace, output_can_workspace)
        else:
            final_output_workspace = output_sample_workspace

        # Tidy up the workspace by removing start/end-NANs and start/end-INFs
        strip_end_nans(final_output_workspace)

        final_output_workspaces.update({reduction_mode: final_output_workspace})

    # Finally add sample log information
    # TODO: Add log information
    return final_output_workspaces


def perform_can_subtraction(sample, can):
    """
    Subtracts the can from the sample workspace.

    We need to manually take care of the q resolution issue here.
    :param sample: the sample workspace
    :param can: the can workspace.
    :return: the subtracted workspace.
    """
    subtraction_name = "Minus"
    subtraction_options = {"LHSWorkspace": sample,
                           "RHSWorkspace": can,
                           SANSConstants.output_workspace: SANSConstants.dummy}
    subtraction_alg = create_unmanaged_algorithm(subtraction_name, **subtraction_options)
    subtraction_alg.execute()
    output_workspace = subtraction_alg.getProperty(SANSConstants.output_workspace).value

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
    _ = can_workspace # noqa
    if sample_workspace.getNumberHistograms() == 1 and sample_workspace.hasDx(0):
        subtracted_workspace.setDx(0, sample_workspace.dataDx(0))


def get_merge_bundle_for_merge_request(output_bundles):
    """
    Get merge the reduction outputs and perform stitching if required
    """
    # Order the reductions. This leaves us with a dict mapping from the reduction type (i.e. HAB, LAB) to
    # a list of reduction settings which contain the information for sample and can.
    reduction_mode_vs_output_bundles = get_reduction_mode_vs_output_bundles(output_bundles)

    # Get the underlying state from one of the elements
    reduction_settings_collection = output_bundles.itervalues().next()
    state = reduction_settings_collection[0].state

    merge_factory = MergeFactory()
    merger = merge_factory.create_merger(state)

    # Run the merger and return the merged output workspace
    return merger.merge(reduction_mode_vs_output_bundles)


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

    :param reduction_setting_bundle: a  ReductionSettingBundle tuple.
    :return: the reduction mode as a string
    """
    # Get the reduction mode
    reduction_mode = reduction_setting_bundle.reduction_mode

    if reduction_mode is ISISReductionMode.Lab:
        reduction_mode_setting = "LAB"
    elif reduction_mode is ISISReductionMode.Hab:
        reduction_mode_setting = "HAB"
    else:
        raise RuntimeError("SingleExecution: An unknown reduction mode was selected: {}. "
                           "Currently only Hab and Lab are supported.".format(reduction_mode))
    return reduction_mode_setting
