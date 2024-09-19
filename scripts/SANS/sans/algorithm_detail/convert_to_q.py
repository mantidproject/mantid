# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
"""Converts a workspace from wavelengths to momentum transfer."""

from math import sqrt

from SANS.sans.common.constants import EMPTY_NAME
from SANS.sans.common.enums import ReductionDimensionality
from SANS.sans.common.general_functions import create_unmanaged_algorithm, append_to_sans_file_tag


def convert_workspace(
    workspace,
    state_convert_to_q,
    output_summed_parts=False,
    wavelength_adj_workspace=None,
    pixel_adj_workspace=None,
    wavelength_and_pixel_adj_workspace=None,
):
    """
    Converts workspace from wavelengths to momentum Transfer
    :param workspace: Workspace in wavelength
    :param state_convert_to_q The convert_to_q field of the SANSState object
    :param wavelength_adj_workspace: (Optional) The Wavelength adjustment workspace
    :param pixel_adj_workspace: (Optional) The pixel adjustment workspace
    :param wavelength_and_pixel_adj_workspace (Optional) The wavelength and pixel adjustment workspace
    :param output_summed_parts: (Optional) If set to true the partial sum of counts and normalisation is returned
    :return: The output workspace in q if output_summed_parts is False. Otherwise a dict containing 'output',
             'counts_summed' and 'norm_summed' with respective workspaces
    """

    # Perform either a 1D reduction or a 2D reduction
    reduction_dimensionality = state_convert_to_q.reduction_dimensionality
    if reduction_dimensionality is ReductionDimensionality.ONE_DIM:
        output_workspace, sum_of_counts_workspace, sum_of_norms_workspace = _run_q_1d(
            workspace,
            output_summed_parts,
            conv_to_q_state=state_convert_to_q,
            pixel_adj_ws=pixel_adj_workspace,
            wavelength_adj_ws=wavelength_adj_workspace,
            wavelength_and_pixel_adj_ws=wavelength_and_pixel_adj_workspace,
        )
    else:
        output_workspace, sum_of_counts_workspace, sum_of_norms_workspace = _run_q_2d(
            workspace,
            output_summed_parts,
            state_convert_to_q=state_convert_to_q,
            wavelength_adj_ws=wavelength_adj_workspace,
            pixel_adj_ws=pixel_adj_workspace,
        )

    # Set the output
    append_to_sans_file_tag(output_workspace, "_convertq")
    if output_summed_parts:
        to_return = {"output": output_workspace, "counts_summed": sum_of_counts_workspace, "norm_summed": sum_of_norms_workspace}
        return to_return
    else:
        return output_workspace


def _run_q_1d(workspace, output_summed_parts, conv_to_q_state, wavelength_adj_ws, pixel_adj_ws, wavelength_and_pixel_adj_ws):
    if conv_to_q_state.use_q_resolution:
        q_resolution_workspace = _create_q_resolution_workspace(convert_to_q=conv_to_q_state, workspace=workspace)
    else:
        q_resolution_workspace = None

    # Extract relevant settings
    q_binning = conv_to_q_state.q_1d_rebin_string
    use_gravity = conv_to_q_state.use_gravity
    gravity_extra_length = conv_to_q_state.gravity_extra_length
    radius_cutoff = conv_to_q_state.radius_cutoff * 1000.0  # Q1D2 expects the radius cutoff to be in mm
    wavelength_cutoff = conv_to_q_state.wavelength_cutoff

    q1d_name = "Q1D"
    q1d_options = {
        "DetBankWorkspace": workspace,
        "OutputWorkspace": EMPTY_NAME,
        "OutputBinning": q_binning,
        "AccountForGravity": use_gravity,
        "RadiusCut": radius_cutoff,
        "WaveCut": wavelength_cutoff,
        "OutputParts": output_summed_parts,
        "ExtraLength": gravity_extra_length,
    }
    if wavelength_adj_ws:
        q1d_options.update({"WavelengthAdj": wavelength_adj_ws})

    if pixel_adj_ws:
        q1d_options.update({"PixelAdj": pixel_adj_ws})

    if wavelength_and_pixel_adj_ws:
        q1d_options.update({"WavePixelAdj": wavelength_and_pixel_adj_ws})

    if q_resolution_workspace:
        q1d_options.update({"QResolution": q_resolution_workspace})

    q1d_alg = create_unmanaged_algorithm(q1d_name, **q1d_options)
    q1d_alg.execute()
    reduced_workspace = q1d_alg.getProperty("OutputWorkspace").value

    # Get the partial workspaces

    if output_summed_parts:
        sum_of_counts_workspace, sum_of_norms_workspace = _get_partial_output(q1d_alg, do_clean=False)
        return reduced_workspace, sum_of_counts_workspace, sum_of_norms_workspace
    else:
        return reduced_workspace, None, None


def _run_q_2d(workspace, output_summed_parts, state_convert_to_q, wavelength_adj_ws, pixel_adj_ws):
    """
    This method performs a 2D data reduction on our workspace.

    Note that it does not perform any q resolution calculation, nor any wavelength-and-pixel adjustment. The
    output workspace contains two numerical axes.
    """

    # Extract relevant settings
    max_q_xy = state_convert_to_q.q_xy_max
    delta_q = state_convert_to_q.q_xy_step
    radius_cutoff = state_convert_to_q.radius_cutoff / 1000.0  # Qxy expects the radius cutoff to be in mm
    wavelength_cutoff = state_convert_to_q.wavelength_cutoff
    use_gravity = state_convert_to_q.use_gravity
    gravity_extra_length = state_convert_to_q.gravity_extra_length

    qxy_name = "Qxy"
    qxy_options = {
        "InputWorkspace": workspace,
        "OutputWorkspace": EMPTY_NAME,
        "MaxQxy": max_q_xy,
        "DeltaQ": delta_q,
        "IQxQyLogBinning": False,  # Log binning disabled for 2D reductions.
        "AccountForGravity": use_gravity,
        "RadiusCut": radius_cutoff,
        "WaveCut": wavelength_cutoff,
        "OutputParts": output_summed_parts,
        "ExtraLength": gravity_extra_length,
    }
    if wavelength_adj_ws:
        qxy_options.update({"WavelengthAdj": wavelength_adj_ws})

    if pixel_adj_ws:
        qxy_options.update({"PixelAdj": pixel_adj_ws})

    qxy_alg = create_unmanaged_algorithm(qxy_name, **qxy_options)
    qxy_alg.execute()

    reduced_workspace = qxy_alg.getProperty("OutputWorkspace").value
    reduced_workspace = _replace_special_values(reduced_workspace)

    # Get the partial workspaces
    if output_summed_parts:
        sum_of_counts_workspace, sum_of_norms_workspace = _get_partial_output(qxy_alg, do_clean=True)
        return reduced_workspace, sum_of_counts_workspace, sum_of_norms_workspace
    else:
        return reduced_workspace, None, None


def _get_partial_output(alg, do_clean=False):
    sum_of_counts_workspace = alg.getProperty("SumOfCounts").value
    sum_of_norms_workspace = alg.getProperty("sumOfNormFactors").value

    if do_clean:
        sum_of_counts_workspace = _replace_special_values(sum_of_counts_workspace)
        sum_of_norms_workspace = _replace_special_values(sum_of_norms_workspace)

    return sum_of_counts_workspace, sum_of_norms_workspace


def _replace_special_values(workspace):
    replace_name = "ReplaceSpecialValues"
    replace_options = {
        "InputWorkspace": workspace,
        "OutputWorkspace": EMPTY_NAME,
        "NaNValue": 0.0,
        "InfinityValue": 0.0,
        "UseAbsolute": False,
        "SmallNumberThreshold": 0.0,
        "SmallNumberValue": 0.0,
        "SmallNumberError": 0.0,
    }
    replace_alg = create_unmanaged_algorithm(replace_name, **replace_options)
    replace_alg.execute()
    return replace_alg.getProperty("OutputWorkspace").value


def _create_q_resolution_workspace(convert_to_q, workspace):
    """
    Provides a q resolution workspace
    :param convert_to_q: a SANSStateConvertToQ object.
    :param workspace: the workspace which is to be reduced.
    :return: a q resolution workspace
    """
    # Load the sigma moderator
    file_name = convert_to_q.moderator_file
    sigma_moderator = _load_sigma_moderator_workspace(file_name)

    # Get the aperture diameters
    a1, a2 = _get_aperture_diameters(convert_to_q)

    # We need the radius, not the diameter in the TOFSANSResolutionByPixel algorithm
    sample_radius = 0.5 * a2
    source_radius = 0.5 * a1

    # The radii and the deltaR are expected to be in mm
    sample_radius *= 1000.0
    source_radius *= 1000.0
    delta_r = convert_to_q.q_resolution_delta_r
    delta_r *= 1000.0

    collimation_length = convert_to_q.q_resolution_collimation_length
    use_gravity = convert_to_q.use_gravity
    gravity_extra_length = convert_to_q.gravity_extra_length

    resolution_name = "TOFSANSResolutionByPixel"
    resolution_options = {
        "InputWorkspace": workspace,
        "OutputWorkspace": EMPTY_NAME,
        "DeltaR": delta_r,
        "SampleApertureRadius": sample_radius,
        "SourceApertureRadius": source_radius,
        "SigmaModerator": sigma_moderator,
        "CollimationLength": collimation_length,
        "AccountForGravity": use_gravity,
        "ExtraLength": gravity_extra_length,
    }
    resolution_alg = create_unmanaged_algorithm(resolution_name, **resolution_options)
    resolution_alg.execute()
    return resolution_alg.getProperty("OutputWorkspace").value


def _load_sigma_moderator_workspace(file_name):
    """
    Gets the sigma moderator workspace.
    :param file_name: the file name of the sigma moderator
    :returns the sigma moderator workspace
    """
    load_name = "LoadRKH"
    load_option = {"Filename": file_name, "OutputWorkspace": EMPTY_NAME, "FirstColumnValue": "Wavelength"}
    load_alg = create_unmanaged_algorithm(load_name, **load_option)
    load_alg.execute()
    moderator_workspace = load_alg.getProperty("OutputWorkspace").value

    convert_name = "ConvertToHistogram"
    convert_options = {"InputWorkspace": moderator_workspace, "OutputWorkspace": EMPTY_NAME}
    convert_alg = create_unmanaged_algorithm(convert_name, **convert_options)
    convert_alg.execute()
    return convert_alg.getProperty("OutputWorkspace").value


def _get_aperture_diameters(convert_to_q):
    """
    Gets the aperture diameters for the sample and the source
    If all fields are specified for a rectangular aperture then this is used, else a circular aperture is
    used.
    :param convert_to_q: a SANSStateConvertToQ object.
    :return: aperture diameter for the source, aperture diameter for the sample
    """

    def set_up_diameter(height, width):
        """
        Prepare the diameter parameter. If there are corresponding H and W values, then
        use them instead. Richard provided the formula: A = 2*sqrt((H^2 + W^2)/6)
        """
        return 2 * sqrt((height * height + width * width) / 6)

    h1 = convert_to_q.q_resolution_h1
    h2 = convert_to_q.q_resolution_h2
    w1 = convert_to_q.q_resolution_w1
    w2 = convert_to_q.q_resolution_w2

    if all(element is not None for element in [h1, h2, w1, w2]):
        a1 = set_up_diameter(h1, w1)
        a2 = set_up_diameter(h2, w2)
    else:
        a1 = convert_to_q.q_resolution_a1
        a2 = convert_to_q.q_resolution_a2

    return a1, a2
