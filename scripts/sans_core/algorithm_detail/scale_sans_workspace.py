# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
"""Multiplies a SANS workspace by an absolute scale and divides it by the sample volume."""

import math

from sans_core.common.constants import EMPTY_NAME
from sans_core.common.enums import SampleShape, SANSInstrument
from sans_core.common.file_information import convert_to_flag

from sans_core.common.general_functions import append_to_sans_file_tag, create_unmanaged_algorithm

DEFAULT_SCALING = 100.0


def scale_workspace(workspace, instrument, state_scale):
    workspace = _multiply_by_abs_scale(instrument, state_scale, workspace)
    workspace = _divide_by_sample_volume(workspace, scale_info=state_scale)
    workspace = _set_sample_values(workspace, state_scale)

    append_to_sans_file_tag(workspace, "_scale")
    return workspace


def _multiply_by_abs_scale(instrument, state_scale, workspace):
    scale_factor = state_scale.scale * DEFAULT_SCALING if state_scale is not None else DEFAULT_SCALING

    if instrument is SANSInstrument.LOQ:
        rescale_to_colette = math.pi
        scale_factor /= rescale_to_colette

    single_valued_name = "CreateSingleValuedWorkspace"
    single_valued_options = {"OutputWorkspace": EMPTY_NAME, "DataValue": scale_factor}
    single_valued_alg = create_unmanaged_algorithm(single_valued_name, **single_valued_options)
    single_valued_alg.execute()
    single_valued_workspace = single_valued_alg.getProperty("OutputWorkspace").value

    multiply_name = "Multiply"
    multiply_options = {"LHSWorkspace": workspace, "RHSWorkspace": single_valued_workspace}
    multiply_alg = create_unmanaged_algorithm(multiply_name, **multiply_options)
    multiply_alg.setPropertyValue("OutputWorkspace", EMPTY_NAME)
    multiply_alg.setProperty("OutputWorkspace", workspace)
    multiply_alg.execute()
    workspace = multiply_alg.getProperty("OutputWorkspace").value
    return workspace


def _divide_by_sample_volume(workspace, scale_info):
    volume = _get_volume(scale_info)

    single_valued_name = "CreateSingleValuedWorkspace"
    single_valued_options = {"OutputWorkspace": EMPTY_NAME, "DataValue": volume}
    single_valued_alg = create_unmanaged_algorithm(single_valued_name, **single_valued_options)
    single_valued_alg.execute()
    single_valued_workspace = single_valued_alg.getProperty("OutputWorkspace").value

    divide_name = "Divide"
    divide_options = {"LHSWorkspace": workspace, "RHSWorkspace": single_valued_workspace}
    divide_alg = create_unmanaged_algorithm(divide_name, **divide_options)
    divide_alg.setPropertyValue("OutputWorkspace", EMPTY_NAME)
    divide_alg.setProperty("OutputWorkspace", workspace)
    divide_alg.execute()
    return divide_alg.getProperty("OutputWorkspace").value


def _set_sample_values(workspace, scale_info):
    sample = workspace.sample()
    sample.setThickness(scale_info.thickness if scale_info.thickness is not None else scale_info.thickness_from_file)
    sample.setWidth(scale_info.width if scale_info.width is not None else scale_info.width_from_file)
    sample.setHeight(scale_info.height if scale_info.height is not None else scale_info.height_from_file)
    sample.setGeometryFlag(convert_to_flag(scale_info.shape if scale_info.shape is not None else scale_info.shape_from_file))
    return workspace


def _get_volume(scale_info):
    thickness = scale_info.thickness if scale_info.thickness is not None else scale_info.thickness_from_file
    width = scale_info.width if scale_info.width is not None else scale_info.width_from_file
    height = scale_info.height if scale_info.height is not None else scale_info.height_from_file
    shape = scale_info.shape if scale_info.shape is not None else scale_info.shape_from_file

    # Now we calculate the volume
    if shape is SampleShape.CYLINDER:
        # Volume = circle area * height
        volume = height * math.pi
        radius = width / 2.0
        volume *= math.pow(radius, 2)
    elif shape is SampleShape.FLAT_PLATE:
        # Flat plate sample
        volume = width * height * thickness
    elif shape is SampleShape.DISC:
        # Factor of four comes from radius = width/2
        # Disc - where height is not used
        volume = thickness * math.pi
        volume *= math.pow(width, 2) / 4.0
    else:
        raise NotImplementedError("DivideByVolumeISIS: The shape {0} is not in the list of " "supported shapes".format(shape))
    return volume
