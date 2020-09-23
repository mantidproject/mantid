# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +

from copy import deepcopy
import numpy as np
import re
from typing import Union

# imports from Mantid
from mantid import AnalysisDataService, mtd
from mantid.dataobjects import TableWorkspace, Workspace2D
from mantid.simpleapi import CloneWorkspace, RenameWorkspace
from Calibration import tube
from Calibration.tube_spec import TubeSpec
from Calibration.tube_calib_fit_params import TubeCalibFitParams


def wire_positions(units: str = 'pixels') -> np.ndarray:
    r"""
    Vertical positions of the standar set of 16 wires. It's assumed that the center of the 16 wires
    coincides with the center of a tube.

    :param units: either one of 'pixels' or 'meters'. If pixels, the bottom of the tube correspons
    to pixel 1. If 'meters', the center of the tube corresponds to the origin of coordinates.

    :raises: AssertionError when incorrect units are passed
    """
    units_valid = ('meters', 'pixels')
    assert units in units_valid, f'units {units} must be one of {units_valid}'
    wire_gap = (2 * 25.4 + 2) / 1000  # gap along the Y-coordinate between consecutive wire centers
    # the center of the 16 wires is aligned with the center of the tube, set to Y == 0
    wire_meters_positions = np.arange(-7.5 * wire_gap, 8.5 * wire_gap, wire_gap)
    if units == 'meters':
        return wire_meters_positions
    tube_length, pixels_per_tube = 0.9001, 256
    wire_pixel_positions = (pixels_per_tube / tube_length) * wire_meters_positions + pixels_per_tube / 2
    return wire_pixel_positions


InputWorkspace = Union[str, Workspace2D]  # allowed types for the input workspace to calibrate_bank


def sufficient_intensity(input_workspace: InputWorkspace, bank_name: str, minimum_intensity=10000) -> bool:
    r"""
    Assert if the average intensity per pixel in the bank surpasses a minimum threshold.

    :param input_workspace:
    :param bank_name:
    :minimum_intensity:
    :return:
    """
    workspace = mtd[str(input_workspace)]
    tube_set = TubeSpec(workspace)
    tube_set.setTubeSpecByString(bank_name)
    workspace_indexes = list()
    for tube_index in range(tube_set.getNumTubes()):
        workspace_indexes_in_tube, skipped = tube_set.getTube(tube_index)
        workspace_indexes.extend(list(workspace_indexes_in_tube))
    return bool(np.mean(workspace.extractY()[workspace_indexes].flatten()) > minimum_intensity)


def fit_bank(workspace: InputWorkspace, bank_name: str, shadow_height: float = 1000, shadow_width: float = 4,
             fit_domain: float = 7, minimum_intensity: float = 1000,
             calibration_table: str = 'CalibTable', peak_pixel_positions_table: str = 'PeakTable') -> None:
    r"""

    :param workspace:
    :param bank_name:
    :param shadow_height:
    :param shadow_width:
    :param fit_domain:
    :param minimum_intensity:
    :param calibration_table:
    :param peak_pixel_positions_table:
    :return:
    """
    message = f'Cannot process workspace {workspace}. Pass the name of an existing workspace or a workspace handle'
    assert isinstance(workspace, (str, Workspace2D)), message
    workspace_name = str(workspace)
    assert AnalysisDataService.doesExist(workspace_name), f'Input workspace {workspace_name} does not exists'
    assert shadow_height > 0, 'shadow height must be positive'
    peak_height, peak_width = -shadow_height, shadow_width
    assert re.match(r'^bank\d+$', bank_name), 'The bank name must be of the form "bankI" where "I" in an integer'
    message = f'Insufficient counts per pixel in workspace {workspace_name} for a confident calibration'
    assert sufficient_intensity(workspace, bank_name, minimum_intensity=minimum_intensity), message
    # Fit only the inner 14 dips because the extrema wires are too close to the tube tips.
    # The dead zone in the tube tips interferes with the shadow cast by the extrema  wires
    # preventing a good fitting
    wire_positions_pixels = wire_positions(units='pixels')[1: -1]
    wire_count = len(wire_positions_pixels)
    peaks_form = [1] * wire_count  # signals we'll be fitting dips (peaks with negative heights)

    fit_par = TubeCalibFitParams(wire_positions_pixels, height=peak_height, width=peak_width, margin=fit_domain)
    fit_par.setAutomatic(True)

    tube.calibrate(workspace_name, bank_name, wire_positions(units='meters')[1: -1],
                   peaks_form, fitPar=fit_par, outputPeak=True)
    if calibration_table != 'CalibTable':
        RenameWorkspace(InputWorkspace='CalibTable', OutputWorkspace=calibration_table)
    if peak_pixel_positions_table != 'PeakTable':
        RenameWorkspace(InputWorkspace='PeakTable', OutputWorkspace=peak_pixel_positions_table)


InputTable = Union[str, TableWorkspace]  # allowed types for the input calibration table to append_bank_number


def append_bank_number(calibration_table: InputTable, bank_name: str, output_table: str = None) -> None:
    r"""
    Add an additional column to the calibration table containing the bank number, same for each pixel.

    :param calibration_table:
    :param bank_name:
    :param output_table: Name of the output table. If `None`, then the input calibration table is overwritten
    :return:
    """
    message = f'Cannot process table {calibration_table}. Pass the name of an existing TableWorkspace' \
              ' or a TableWorkspace handle'
    assert isinstance(calibration_table, (str, TableWorkspace)), message
    if output_table is not None:
        CloneWorkspace(InputWorkspace=calibration_table, OutputWorkspace=output_table)
    else:
        output_table = str(calibration_table)


def criterium_peak_pixel_position(peak_table: InputTable, zscore_threshold: float = 2.5,
                                  deviation_threshold: float = 3.0) -> np.ndarray:
    r"""
    Flag tubes whose peak pixel positions deviate considerably from the peak pixel positions when
    averaged for all tubes in the bank.

    .. math::

      <p_i> = \frac{1}{n_t} \Sum_{j=1}^{n_t} p_{ij}
      \delta_j^2 = \frac{1}{n_w} \Sum (p_{ij} - <p_i>)^2
      assert d_j < threshold

    :param peak_table: pixel positions of the peaks for each tube
    :param zscore_threshold: maximum Z-score for the pixels positions of a tube.
    :param deviation_threshold: maximum deviation (in pixels) for the pixels positions of a tube.
    :return: array of booleans, one per tube. `True` is the tube passes the acceptance criterium, `False` otherwise.
    """
    table = mtd[str(peak_table)]  # handle to the peak table
    peak_count = table.columnCount() - 1  # the first column contains the names of the tubes
    # `positions_average` stores the pixel position for each peak, averaged for all tubes
    positions_average = [np.mean(table.column(column_number)) for column_number in range(1, 1 + peak_count)]

    deviations = list()  # a measure of how much the peak positions in a tube deviate from the mean positions
    tube_count = table.rowCount()  # number of tubes in the bank
    for tube_index in range(tube_count):
        positions = np.array(list(table.row(tube_index).values())[1:])  # peak positions for the current tube
        deviations.append(np.sqrt(np.mean(np.square(positions - positions_average))))

    # find tubes with a large Z-score
    outlier_values = list()
    values = deepcopy(deviations)
    z_score = 1000
    outlier_value = 1000
    deviation_threshold = 3.0  # three pixels
    while z_score > zscore_threshold and outlier_value > deviation_threshold and len(values) > 0:
        # find the tube with the highest Z-score, possibly signaling a large deviation from the mean
        mean, std = np.mean(values), np.std(values)
        outlier_index = np.argmax(np.abs((values - mean) / std))
        outlier_value = values[outlier_index]
        # recalculate the Z-score of the tube, but removing it from the pool of values. This removes
        # any skewing effects from including the aberrant tube in the calculation of its Z-score
        del values[outlier_index]
        mean, std = np.mean(values), np.std(values)
        z_score = np.abs((outlier_value - mean) / std)
        if z_score > zscore_threshold and outlier_value > deviation_threshold:
            outlier_values.append(outlier_value)

    # flag the outlier tubes as failing the criterium
    criterium_pass = np.tile(True, tube_count)  # initialize as all tubes passing the criterium
    if len(outlier_values) > 0:
        failure_indexes = [deviations.index(value) for value in outlier_values]
        criterium_pass[failure_indexes] = False

    return criterium_pass
