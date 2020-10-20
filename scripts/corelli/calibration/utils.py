# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +

from os import path
import numpy as np
from numpy.polynomial.polynomial import polyval
from typing import List, Optional, Union

# imports from Mantid
from mantid.api import AnalysisDataService, mtd, WorkspaceGroup
from mantid.dataobjects import TableWorkspace, Workspace2D
from mantid.simpleapi import (ApplyCalibration, CloneWorkspace, CreateEmptyTableWorkspace, Integration, LoadEventNexus,
                              LoadNexusProcessed, RenameWorkspace, UnGroupWorkspace)
try:
    from mantidqt.widgets.instrumentview.presenter import InstrumentViewPresenter
    from mantidqt.utils.qt.qappthreadcall import QAppThreadCall
except (ImportError, ModuleNotFoundError):
    InstrumentViewPresenter, QAppThreadCall = None, None
from Calibration import tube
from Calibration.tube_calib_fit_params import TubeCalibFitParams

# Type aliases
InputTable = Union[str, TableWorkspace]  # allowed types for the input calibration table to append_bank_number
WorkspaceTypes = Union[str, Workspace2D]  # allowed types for the input workspace to calibrate_bank
WorkspaceGroupTypes = Union[str, WorkspaceGroup]

PIXELS_PER_TUBE = 256
TUBES_IN_BANK = 16
TUBE_LENGTH = 0.900466  # in meters
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
    Expand a bank selection string into a list of bank numbers

    :param bank_selection: selection string, such as '10,12-15,17-21'
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
    Load events only for the selected banks, and don't load metadata.

    If the file is not an events file, but a Nexus processed file, the bank_selection is ignored.
    :param filename: Filename to an Event nexus file or a processed nexus file
    :param bank_selection: selection string, such as '10,12-15,17-21'
    :param output_workspace: name of the output workspace containing counts per pixel
    :return: workspace containing counts per pixel. Events in each pixel are integrated into neutron counts.
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


def calculate_peak_y_table(peak_table: InputTable,
                           parameters_table_group: WorkspaceGroupTypes,
                           output_workspace: str = 'PeakYTable') -> TableWorkspace:
    r"""
    Evaluate the Y-coordinate of each wire shadow given the estimated pixel positions of the
    shadows and the polynomial function translating pixel positions to y-coordinate

    :param peak_table:
    :param parameters_table_group:
    :param output_workspace:

    :return: TableWorkspace
    """
    # Number of rows of peak_table must be the number of tables within parameters_table_group
    # Create empty PeakYTable, see tube_calib.getCalibration
    # Iterate over the rows of peak_table, and tables of parameters_table_group
    #   Extract peak positions in pixel units
    #   Extract polynomial coefficients, see tube_calib.correct_tube_to_ideal_tube
    #   Evaluate the Y-coordinate for the peak positions, see tube_calib.correct_tube_to_ideal_tube
    #   Append row to PeakYTable, see tube_calib.getCalibration
    # Return the handle to PeakYTable
    peak_workspace, parameters_workspace = mtd[str(peak_table)], mtd[str(parameters_table_group)]
    error_message = 'number of rows in peak_table different than number of tables in parameters_table_group'
    assert peak_workspace.rowCount() == parameters_workspace.getNumberOfEntries(), error_message

    # Create empty peak_y_table with same column names as those of peak_table
    peak_y_workspace = CreateEmptyTableWorkspace(OutputWorkspace=output_workspace)
    for column_type, column_name in zip(peak_workspace.columnTypes(), peak_workspace.getColumnNames()):
        peak_y_workspace.addColumn(type=column_type, name=column_name)
    tube_names = peak_workspace.column(0)  # e.g., ['CORELLI/A row/bank10/sixteenpack/tube1',...]

    # calculate the vertical positions for each tube, and append to peak_y_workspace
    for tube_index, (row, table) in enumerate(zip(peak_workspace, parameters_workspace)):
        peak_pixel_positions = list(row.values())[1:]  # peaks in pixel units for the current tube
        coefficients = table.column(1)[:-1]  # A0, A1, A2 coefficients
        peak_vertical_positions = polyval(peak_pixel_positions, coefficients)  # peaks along vertical axes
        peak_y_workspace.addRow([tube_names[tube_index]] + peak_vertical_positions.tolist())

    return peak_y_workspace


def calibrate_tube(workspace: WorkspaceTypes,
                   tube_name: str,
                   output_peak_table: str = 'PeakTable',
                   output_parameters_table: str = 'ParametersTable',
                   output_peak_y_table: str = 'PeakYTable',
                   shadow_height: float = 1000,
                   shadow_width: float = 4,
                   fit_domain: float = 7) -> TableWorkspace:
    r"""
    Calibration table for one tube of CORELLI

    This function creates TableWorkspace 'CalibTable', TableWorkspace 'PeakTable',
    and WorkspaceGroup 'ParametersTable' containing TableWorkspace 'ParametersTableI'
    where 'I' is the tube number.

    :param workspace: string or handle to ~mantid.dataobjects.Workspace2D
    :param tube_name: string uniquely representing one tube e.g. 'bank88/sixteenpack/tube3'
    :param output_peak_table:
    :param output_parameters_table:
    :param output_peak_y_table:
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
    # Initial guess for the peak positions, assuming:
    # - the center of the the wire mesh coincides with the center ot the tube_calib_fit_params
    # - wires cast a shadow on a perfectly calibrated tube
    fit_extent = (fit_domain / PIXELS_PER_TUBE) * TUBE_LENGTH  # fit domain in meters
    assert fit_extent < WIRE_GAP, 'The fit domain cannot be larger than the distance between consecutive wires'
    wire_pixel_positions = wire_positions(units='pixels')[1: -1]
    fit_par = TubeCalibFitParams(wire_pixel_positions, height=peak_height, width=peak_width, margin=fit_domain)
    fit_par.setAutomatic(True)
    # Generate the calibration table, the peak table, and the parameters table
    peaks_form = [1] * len(wire_pixel_positions)  # signals we'll be fitting dips (peaks with negative heights)
    calibration_table, _ = tube.calibrate(workspace, tube_name, wire_positions(units='meters')[1: -1], peaks_form,
                                          fitPar=fit_par, outputPeak=True,
                                          parameters_table_group='ParametersTableGroup')
    # Additional workspaces
    # Table with shadow positions along the tube, in pixel units
    if output_peak_table != 'PeakTable':  # 'PeakTable' is output by tube.calibrate
        RenameWorkspace(InputWorkspace='PeakTable', OutputWorkspace=output_peak_table)
    # Table with shadow positions along the vertical axis
    calculate_peak_y_table(output_peak_table, 'ParametersTableGroup', output_workspace=output_peak_y_table)
    # Table with optimized parameters for the polynomial coefficients A0, A1, A2, and chi-square
    RenameWorkspace(InputWorkspace=mtd['ParametersTableGroup'].getItem(0), OutputWorkspace=output_parameters_table)
    UnGroupWorkspace(InputWorkspace='ParametersTableGroup')
    return calibration_table


def apply_calibration(workspace: WorkspaceTypes, calibration_table: InputTable,
                      output_workspace: Optional[str] = None, show_instrument: bool = False) -> Workspace2D:
    r"""
    Calibrate the detector positions with an input table, and open the instrument view if so requested.

    :param workspace: input Workspace2D containing total neutron counts per pixel
    :param calibration_table: a TableWorskpace containing one column for detector ID and one column
    for its calibrated XYZ coordinates, in meters
    :param output_workspace: name of the output workspace containing calibrated detectors. If `None`, then
        the output workspace name will be the input workspace plus the suffix `_calibrated`
    :param show_instrument: open the instrument view for `output_workspace`

    :raises AssertionError: either `workspace` or `calibration_table` are not found
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
