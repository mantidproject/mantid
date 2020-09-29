# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +

from os import path
import numpy as np
from typing import List, Optional, Union

# imports from Mantid
from mantid.api import AnalysisDataService, mtd
from mantid.dataobjects import TableWorkspace, Workspace2D
from mantid.simpleapi import ApplyCalibration, CloneWorkspace, Integration, LoadEventNexus, LoadNexusProcessed
try:
    from mantidqt.widgets.instrumentview.presenter import InstrumentViewPresenter
    from mantidqt.utils.qt.qappthreadcall import QAppThreadCall
except ModuleNotFoundError:
    InstrumentViewPresenter, QAppThreadCall = None, None
from Calibration import tube
from Calibration.tube_calib_fit_params import TubeCalibFitParams

# Type aliases
InputTable = Union[str, TableWorkspace]  # allowed types for the input calibration table to append_bank_number
WorkspaceTypes = Union[str, Workspace2D]  # allowed types for the input workspace to calibrate_bank

PIXELS_PER_TUBE = 256
TUBES_IN_BANK = 16
TUBE_LENGTH = 0.9001  # in meters
WIRE_GAP = 52.8 / 1000  # in meters, distance between consecutive wires


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
    wire_pixel_positions = (PIXELS_PER_TUBE / TUBE_LENGTH) * wire_meters_positions + PIXELS_PER_TUBE / 2
    return wire_pixel_positions


def bank_numbers(bank_selection: str) -> List[str]:
    r"""

    :param bank_selection:
    :return:
    """
    banks = list()  # list of bank numbers, as string
    ranges = [r.strip() for r in bank_selection.split(',')]  # split by comma
    for r in ranges:
        if '-' in r:
            start, end = [int(n.strip()) for n in r.split('-')]
            banks.extend([str(n) for n in range(start, end + 1)])
        else:
            banks.append(r)
    return banks


def load_banks(filename: str, bank_selection: str, output_workspace: str) -> Workspace2D:
    r"""

    :param filename: Filename to an Event nexus file or a processed nexus file
    :param bank_selection:
    :param output_workspace:
    :return:
    """
    assert path.exists(filename), f'File {filename} does not exist'
    bank_names = ','.join(['bank' + b for b in bank_numbers(bank_selection)])
    try:
        LoadEventNexus(Filename=filename, OutputWorkspace=output_workspace,
                       BankName=bank_names, LoadMonitors=False, LoadLogs=False)
    except (RuntimeError, ValueError):
        LoadNexusProcessed(Filename=filename, OutputWorkspace=output_workspace)
    Integration(InputWorkspace=output_workspace, OutputWorkspace=output_workspace)
    return mtd[output_workspace]


def calculate_tube_calibration(workspace: WorkspaceTypes, tube_name: str, shadow_height: float = 1000,
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
    # the center of the 16 wires is aligned with the center of the tube, set to Y == 0
    bottom_wire_position, topmost_wire_position = -7.5 * WIRE_GAP, 8.5 * WIRE_GAP
    wire_positions = np.arange(bottom_wire_position, topmost_wire_position, WIRE_GAP)
    # Fit only the inner 14 dips because the extrema wires are too close to the tube tips.
    # The dead zone in the tube tips interferes with the shadow cast by the extrema  wires
    # preventing a good fitting
    wire_positions = wire_positions[1: -1]  # drop the extrema wires
    wire_count = len(wire_positions)
    peaks_form = [1] * wire_count  # signals we'll be fitting dips (peaks with negative heights)
    # Initial guess for the peak positions, assuming:
    # - the center of the the wire mesh coincides with the center ot the tube_calib_fit_params
    # - wires cast a shadow on a perfectly calibrated tube
    fit_extent = (fit_domain / PIXELS_PER_TUBE) * TUBE_LENGTH  # fit domain in meters
    assert fit_extent < WIRE_GAP, 'The fit domain cannot be larger than the distance between consecutive wires'
    wire_pixel_positions = (PIXELS_PER_TUBE / TUBE_LENGTH) * wire_positions + PIXELS_PER_TUBE / 2
    fit_par = TubeCalibFitParams(wire_pixel_positions, height=peak_height, width=peak_width, margin=fit_domain)
    fit_par.setAutomatic(True)
    calibration_table, _ = tube.calibrate(workspace, tube_name, wire_positions, peaks_form, fitPar=fit_par,
                                          outputPeak=True)
    return calibration_table


def calibrate_instrument(workspace: WorkspaceTypes, calibration_table: InputTable,
                         output_workspace: Optional[str] = None, show_instrument: bool = False) -> Workspace2D:
    r"""
    Calibrate an instrument, and show it if requested

    :param workspace:
    :param calibration_table:
    :param output_workspace:
    :param show_instrument:
    :return:
    """
    assert AnalysisDataService.doesExist(str(workspace)), f'No worksapce {str(workspace)} found'
    assert AnalysisDataService.doesExist(str(calibration_table)), f'No table {str(calibration_table)} found'
    if output_workspace is None:
        output_workspace = str(workspace) + '_calibrated'

    CloneWorkspace(InputWorkspace=workspace, OutputWorkspace=output_workspace)
    ApplyCalibration(Workspace=output_workspace, CalibrationTable=calibration_table)

    if show_instrument is True and None not in (InstrumentViewPresenter, InstrumentViewPresenter):
        instrument_presenter = QAppThreadCall(InstrumentViewPresenter)(mtd[output_workspace])
        QAppThreadCall(instrument_presenter.show_view)()

    return mtd[output_workspace]
