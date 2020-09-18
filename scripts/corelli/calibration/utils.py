# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +

import numpy as np
from typing import Union

# imports from Mantid
from mantid.dataobjects import TableWorkspace, Workspace2D
from Calibration import tube
from Calibration.tube_calib_fit_params import TubeCalibFitParams

InputWorkspace = Union[str, Workspace2D]  # type alias


def calculate_tube_calibration(workspace: InputWorkspace, tube_name: str, shadow_height: float = 1000,
                               shadow_width: float = 4, fit_domain: float = 7) -> TableWorkspace:
    r"""
    Calibration table for one tube of CORELLI

    :param workspace: string or handle to ~mantid.dataobjects.Workspace2D
    :param tube_name: string uniquely representing one tube e.g. 'bank88/sixteenpack/tube3'
    :param shadow_height: estimated dip in the background intensity.
    :param shadow_width: estimated width of the shadow cast by the wire, in pixel units
    :param fit_domain: estimated range, in pixel units, over which to carry out the fit.

    :return: table containing detector ID and position vector
    """
    message = f'Cannot process workspace {workspace}. Pass the name of an existing workspace or a workspace handle'
    assert isinstance(workspace, (str, Workspace2D)), message
    assert shadow_height > 0, 'shadow height must be positive'
    for marker in ('bank', 'sixteenpack', 'tube'):
        assert marker in tube_name, f'{tube_name} does not uniquely specify one tube'
    peak_height, peak_width = -shadow_height, shadow_width
    wire_gap = (2 * 25.4 + 2) / 1000  # gap along the Y-coordinate between consecutive wire centers
    # the center of the 16 wires is aligned with the center of the tube, set to Y == 0
    wire_positions = np.arange(-7.5 * wire_gap, 8.5 * wire_gap, wire_gap)
    # Fit only the inner 14 dips because the extrema wires are too close to the tube tips.
    # The dead zone in the tube tips interferes with the shadow cast by the extrema  wires
    # preventing a good fitting
    wire_positions = wire_positions[1: -1]  # drop the extrema wires
    wire_count = len(wire_positions)
    peaks_form = [1] * wire_count  # signals we'll be fitting dips (peaks with negative heights)
    # Initial guess for the peak positions, assuming:
    # - the center of the the wire mesh coincides with the center ot the tube_calib_fit_params
    # - wires cast a shadow on a perfectly calibrated tube
    tube_length, pixels_per_tube = 0.9001, 256
    fit_extent = (fit_domain / pixels_per_tube) * tube_length  # fit domain in meters
    assert fit_extent < wire_gap, 'The fit domain cannot be larger than the distance between consecutive wires'
    wire_pixel_positions = (pixels_per_tube / tube_length) * wire_positions + pixels_per_tube / 2
    fit_par = TubeCalibFitParams(wire_pixel_positions, height=peak_height, width=peak_width, margin=fit_domain)
    fit_par.setAutomatic(True)
    return tube.calibrate(workspace, tube_name, wire_positions, peaks_form, fitPar=fit_par)
