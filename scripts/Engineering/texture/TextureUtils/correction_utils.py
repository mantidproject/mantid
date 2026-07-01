# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2026 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +

from Engineering.texture.correction.correction_model import TextureCorrectionModel
from mantid.simpleapi import logger
from typing import Sequence, Tuple


def run_abs_corr(
    wss: Sequence[str],
    ref_ws: str | None = None,
    orientation_file: str | None = None,
    orient_file_is_euler: bool | None = None,
    euler_scheme: str | None = None,
    euler_axes_sense: str | None = None,
    copy_ref: bool = False,
    include_abs_corr: bool = False,
    monte_carlo_args: str | None = None,
    gauge_vol_preset: str | None = None,
    gauge_vol_shape_file: str | None = None,
    include_atten_table: bool = False,
    eval_point: str | float | None = None,
    eval_units: str | None = None,
    root_dir: str = ".",
    include_div_corr: bool = False,
    div_hoz: float | None = None,
    div_vert: float | None = None,
    det_hoz: float | None = None,
    clear_ads_after: bool = True,
) -> None:
    """
    Apply absorption correction to data for use in texture analysis pipeline
    wss: Sequence of workspace names to have corrections calculated and applied
    ref_ws: Name of the reference workspace, if one required
    orientation_file: path to the orientation file (should be .txt with one line per run)
    orient_file_is_euler: flag for whether the file provides euler goniometer angles or direct rotation matrices
    euler_scheme: the lab frame directions along which each axis of the goniometer initially lies
    euler_axes_sense: the sense of the rotation around each of the axes (1 being CCW, -1 being CW)
    copy_ref: Whether the reference sample should be copied to each ws
    include_abs_corr: Whether the workspaces should have the absorption correction applied
    monte_carlo_args: String of arguments to supply to the MonteCarloAbsorption alg e.g. "Arg1: val1, Arg2: val2"
    gauge_vol_preset: Name of the preset to use for the gauge volume, currently ("4mmCube"), otherwise should be Custom or No Gauge Volume
    gauge_vol_shape_file: Path to custom gauge volume shape file
    include_atten_table: flag for whether a table of attenuation values at a specified point should be created
    eval_point: point to calculate the attenuation coefficient at
    eval_units: units which the eval_point is given in
    root_dir: Directory path in which the experiment directory is constructed
    include_div_corr: Flag for whether to include a beam divergence correction
    div_hoz: Value of beam divergence in the horizontal plane
    div_vert: Value of beam divergence in the vertical plane
    det_hoz: Value of divergence on the detector in the horizontal plane
    clear_ads_after: Flag for whether the produced files should be removed from the ADS after they have been saved
    """
    model = TextureCorrectionModel()
    model.set_reference_ws(ref_ws)

    valid_inputs, error_msg = validate_abs_corr_inputs(
        ref_ws,
        orientation_file,
        orient_file_is_euler,
        euler_scheme,
        euler_axes_sense,
        copy_ref,
        include_abs_corr,
        gauge_vol_preset,
        gauge_vol_shape_file,
        include_atten_table,
        eval_point,
        eval_units,
        include_div_corr,
        div_hoz,
        div_vert,
        det_hoz,
    )
    if not valid_inputs:
        logger.error(error_msg)
        return
    # otherwise run script
    if orientation_file:
        model.load_all_orientations(wss, orientation_file, orient_file_is_euler, euler_scheme, euler_axes_sense)

    out_wss = [f"Corrected_{ws}" for ws in wss]

    if copy_ref:
        model.copy_sample_info(ref_ws, wss)

    model.set_include_abs(include_abs_corr)
    model.set_include_atten(include_atten_table)
    model.set_include_div(include_div_corr)
    model.set_remove_after_processing(clear_ads_after)

    abs_args = {"gauge_vol_preset": gauge_vol_preset, "gauge_vol_file": gauge_vol_shape_file, "mc_param_str": monte_carlo_args}

    atten_args = {"atten_val": eval_point, "atten_units": eval_units}

    div_args = {"hoz": div_hoz, "vert": div_vert, "det_hoz": det_hoz}

    model.calc_all_corrections(wss, out_wss, root_dir=root_dir, abs_args=abs_args, atten_args=atten_args, div_args=div_args)


def validate_abs_corr_inputs(
    ref_ws: str | None = None,
    orientation_file: str | None = None,
    orient_file_is_euler: bool | None = None,
    euler_scheme: str | None = None,
    euler_axes_sense: str | None = None,
    copy_ref: bool = False,
    include_abs_corr: bool = False,
    gauge_vol_preset: str | None = None,
    gauge_vol_shape_file: str | None = None,
    include_atten_table: bool = False,
    eval_point: str | float | None = None,
    eval_units: str | None = None,
    include_div_corr: bool = False,
    div_hoz: float | None = None,
    div_vert: float | None = None,
    det_hoz: float | None = None,
) -> Tuple[bool, str]:
    error_msg = ""
    # validate inputs
    if orientation_file:
        valid_orientation_inputs = isinstance(orient_file_is_euler, bool)
        if not valid_orientation_inputs:
            error_msg += r"If orientation file is specified, must flag orient_file_is_euler.\n"
        if valid_orientation_inputs and orient_file_is_euler:
            # if is euler flag, require euler_scheme and euler_axes_sense
            valid_orientation_inputs = isinstance(euler_scheme, str) and isinstance(euler_axes_sense, str)
            if not valid_orientation_inputs:
                error_msg += r"If orientation file is euler, must provide scheme and sense.\n"

    if copy_ref:
        if not isinstance(ref_ws, str):
            error_msg += r"If copy_ref is True, must provide ref_ws.\n"

    if include_abs_corr:
        if gauge_vol_preset == "Custom":
            if not isinstance(gauge_vol_shape_file, str):
                error_msg += r"If custom gauge volume required, must provide shape xml as file.\n"

    if include_atten_table:
        if not (isinstance(eval_point, str | float) and isinstance(eval_units, str)):
            error_msg += r"If attenuation table required, must provide valid point and units.\n"

    if include_div_corr:
        if not (isinstance(div_hoz, float) and isinstance(div_vert, float) and isinstance(det_hoz, float)):
            error_msg += r"If divergence correction required, must provide valid values.\n"
    # if error_msg is still empty string, the inputs are assumed to be valid
    return error_msg == "", error_msg
