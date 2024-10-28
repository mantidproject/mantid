# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
"""
This file is concerned with calibrating a specified set of tubes

The main function is :func:`getCalibration` which is at the end of this file.
It populates an empty Calibration Table Workspace with the new positions of the pixel detectors after calibration.
This Calibration Table Workspace can be used later to move the pixel detectors to the calibrated positions.

Users should not need to directly call any other function other than :func:`getCalibration` from this file.

"""
## Author: Karl palmen ISIS and for readPeakFile Gesner Passos ISIS

# Standard and third-party
import copy
import math
import numpy
import os
import re
from typing import Any, Dict, List, Optional, Tuple, Union

# Mantid
from mantid.api import AnalysisDataService as ADS
from mantid.dataobjects import EventWorkspace, TableWorkspace, Workspace2D
from mantid.simpleapi import CloneWorkspace, CreateWorkspace, DeleteWorkspace, Fit, GroupWorkspaces, RenameWorkspace
from mantid.kernel import config

# Calibration
from ideal_tube import IdealTube
from tube_calib_fit_params import TubeCalibFitParams
from tube_spec import TubeSpec

# Type aliases
ArrayInt = Union[List[int], numpy.ndarray]
WorkspaceInput = Union[str, EventWorkspace, Workspace2D]


def create_tube_calibration_ws_by_ws_index_list(integrated_workspace, output_workspace, workspace_index_list):
    """
    Creates workspace with integrated data for one tube against distance along tube
    The tube is specified by a list of workspace indices of its spectra

    @param integrated_workspace: Workspace of integrated data
    @param workspace_index_list:  list of workspace indices for the tube
    @param x_unit: unit of distance ( Pixel)
    @param show_plot: True = show plot of workspace created, False = just make the workspace.

    Return Value: Workspace created

    """

    n_spectra = len(workspace_index_list)
    if n_spectra < 1:
        return
    pixel_numbers = []
    integrated_pixel_counts = []
    pixel = 1
    # integratedWorkspace.
    for i in workspace_index_list:
        pixel_numbers.append(pixel)
        pixel = pixel + 1
        integrated_pixel_counts.append(integrated_workspace.dataY(i)[0])

    CreateWorkspace(dataX=pixel_numbers, dataY=integrated_pixel_counts, OutputWorkspace=output_workspace)
    # For some reason plotSpectrum is not recognised, but instead we can plot this workspace afterwards.


# Return the udet number and [x,y,z] position of the detector (or virtual detector) corresponding to spectra spectra_number
# Thanks to Pascal Manuel for this function
def get_detector_pos(work_handle, spectra_number):
    udet = work_handle.getDetector(spectra_number)
    return udet.getID(), udet.getPos()


# Given the center of a slit in pixels return the interpolated y
#  Converts from pixel coords to Y.
#     If a pixel coord is not integer
#     it is effectively rounded to half integer before conversion, rather than interpolated.
#     It allows the pixel widths to vary (unlike correctTube).
# Thanks to Pascal Manuel for this function
def get_ypos(work_handle, pixel_float):
    center_low_pixel = int(math.floor(pixel_float))
    center_high_pixel = int(math.ceil(pixel_float))
    idlow, low = get_detector_pos(work_handle, center_low_pixel)  # Get the detector position of the nearest lower pixel
    idhigh, high = get_detector_pos(work_handle, center_high_pixel)  # Get the detector position of the nearest higher pixel
    center_y = (center_high_pixel - pixel_float) * low.getY() + (pixel_float - center_low_pixel) * high.getY()
    center_y /= center_high_pixel - center_low_pixel
    return center_y


def fit_gaussian_params(height, centre, sigma):  # Compose string argument for fit
    return "name=Gaussian, Height={0}, PeakCentre={1}, Sigma={2}".format(height, centre, sigma)


def fit_end_erfc_params(B, C):  # Compose string argument for fit
    return "name=EndErfc, B={0}, C={1}".format(B, C)


#
# definition of the functions to fit
#


def fit_edges(fit_par, index, ws, output_ws):
    # find the edge position
    centre = fit_par.getPeaks()[index]
    outer_edge, inner_edge, end_grad = fit_par.getEdgeParameters()
    margin = fit_par.getMargin()
    # get values around the expected center
    all_values = ws.dataY(0)
    right_limit = len(all_values)
    values = all_values[max(int(centre - margin), 0) : min(int(centre + margin), len(all_values))]

    # identify if the edge is a sloping edge or descent edge
    descent_mode = values[0] > values[-1]
    if descent_mode:
        start = max(centre - outer_edge, 0)
        end = min(centre + inner_edge, right_limit)
        edgeMode = -1
    else:
        start = max(centre - inner_edge, 0)
        end = min(centre + outer_edge, right_limit)
        edgeMode = 1
    Fit(InputWorkspace=ws, Function=fit_end_erfc_params(centre, end_grad * edgeMode), StartX=str(start), EndX=str(end), Output=output_ws)
    return 1  # peakIndex (center) -> parameter B of EndERFC


def fit_gaussian(fit_par, index, ws, output_ws):
    # find the peak position
    centre = fit_par.getPeaks()[index]
    margin = fit_par.getMargin()

    # get values around the expected center
    all_values = ws.dataY(0)

    right_limit = len(all_values)

    min_index = max(int(centre - margin), 0)
    max_index = min(int(centre + margin), right_limit)
    values = all_values[min_index:max_index]

    # find the peak position
    if fit_par.getAutomatic():
        # find the parameters for fit dynamically
        max_value = numpy.max(values)
        min_value = numpy.min(values)
        half = (max_value - min_value) * 2 / 3 + min_value
        above_half_line = len(numpy.where(values > half)[0])
        beyond_half_line = len(values) - above_half_line
        if above_half_line < beyond_half_line:
            # means that there are few values above the midle, so it is a peak
            centre = numpy.argmax(values) + min_index
            background = min_value
            height = max_value - background
            width = len(numpy.where(values > height / 2 + background))
        else:
            # means that there are many values above the midle, so it is a trough
            centre = numpy.argmin(values) + min_index
            background = max_value
            height = min_value - max_value  # negative value
            width = len(numpy.where(values < min_value + height / 2))

        start = max(centre - margin, 0)
        end = min(centre + margin, right_limit)

        fit_msg = "name=LinearBackground,A0=%f;name=Gaussian,Height=%f,PeakCentre=%f,Sigma=%f" % (background, height, centre, width)

        Fit(InputWorkspace=ws, Function=fit_msg, StartX=str(start), EndX=str(end), Output=output_ws)

        peak_index = 3

    else:
        # get the parameters from fitParams
        background = 1000
        height, width = fit_par.getHeightAndWidth()
        start = max(centre - margin, 0)
        end = min(centre + margin, right_limit)

        # fit the input data as a linear background + gaussian fit
        # it was seen that the best result for static general fitParamters,
        # is to divide the values in two fitting steps
        Fit(InputWorkspace=ws, Function="name=LinearBackground,A0=%f" % background, StartX=str(start), EndX=str(end), Output="Z1")
        Fit(
            InputWorkspace="Z1_Workspace",
            Function="name=Gaussian,Height=%f,PeakCentre=%f,Sigma=%f" % (height, centre, width),
            WorkspaceIndex=2,
            StartX=str(start),
            EndX=str(end),
            Output=output_ws,
        )
        CloneWorkspace(output_ws + "_Workspace", OutputWorkspace="gauss_" + str(index))
        peak_index = 1

    return peak_index


def getPoints(integrated_ws, func_forms, fit_params, which_tube, show_plot=False):
    """
    Get the centres of N slits or edges for calibration

    It does look for the peak position in pixels by fitting the peaks and
    edges. It is the method responsible for estimating the peak position in each tube.

    .. note::
      This N slit method is suited for WISH or the five sharp peaks of MERLIN .

    :param integrated_ws: Workspace of integrated data
    :param func_forms: array of function form 1=slit/bar, 2=edge
    :param fit_params: a TubeCalibFitParams object contain the fit parameters
    :param which_tube:  a list of workspace indices for one tube (define a single tube)
    :param show_plot: show plot for this tube

    :rtype: array of the slit/edge positions (-1.0 indicates failed to find position)

    """

    # Create input workspace for fitting
    # get all the counts for the integrated workspace inside the tube
    counts_y = numpy.array([integrated_ws.dataY(i)[0] for i in which_tube])
    if len(counts_y) == 0:
        return
    get_points_ws = CreateWorkspace(range(len(counts_y)), counts_y, OutputWorkspace="TubePlot")
    calib_points_ws = "CalibPoint"
    results = []
    fitt_y_values = []
    fitt_x_values = []

    # Loop over the points
    for i in range(len(func_forms)):
        if func_forms[i] == 2:
            # find the edge position
            peak_index = fit_edges(fit_params, i, get_points_ws, calib_points_ws)
        else:
            peak_index = fit_gaussian(fit_params, i, get_points_ws, calib_points_ws)
        peak_centre = tuple(ADS.retrieve(calib_points_ws + "_Parameters").row(peak_index).items())[1][1]
        results.append(peak_centre)

        if show_plot:
            ws = ADS.retrieve(calib_points_ws + "_Workspace")
            fitt_y_values.append(copy.copy(ws.dataY(1)))
            fitt_x_values.append(copy.copy(ws.dataX(1)))

    if show_plot:
        CreateWorkspace(OutputWorkspace="FittedData", DataX=numpy.hstack(fitt_x_values), DataY=numpy.hstack(fitt_y_values))
    return results


def get_ideal_tube_from_n_slits(integrated_workspace, slits):
    """
    Given N slits for calibration on an ideal tube
    convert to Y values to form a ideal tube for correctTubeToIdealTube()

    @param integrated_workspace: Workspace of integrated data
    @param slits: positions of slits for ideal tube (in pixels)

    Return Value: Ideal tube in Y-coords for use by correctTubeToIdealTube()

    """
    ideal = []
    for i in range(len(slits)):
        ideal.append(get_ypos(integrated_workspace, slits[i]))  # Use Pascal Manuel's Y conversion.

    return ideal


def correct_tube(AP, BP, CP, nDets):
    """
    Corrects position errors in a tube in the same manner as is done for MERLIN
    according to an algorithm used by Rob Bewley in his MATLAB code.

    @param AP: Fit position of left (in pixels)
    @param BP: Fit position of right (in pixels)
    @param CP: Fit position of centre (in pixels)
    @param nDets: Number of pixel detectors in tube

    Return Value: Array of corrected Xs  (in pixels)
    """

    AO = AP / (nDets - AP)
    BO = (nDets - BP) / BP
    # First correct centre point for offsets
    CPN = CP - (AO * (nDets - CP)) + BO * CP
    x = []
    for i in range(nDets):
        xi = i + 1.0
        x.append(xi - ((nDets - xi) * AO) + (xi * BO))  # this is x corrected for offsets

    # Now calculate the gain error
    gain_error = ((nDets + 1) / 2.0 - CPN) / (CPN * (nDets - CPN))
    x_bin_new = []
    for i in range(nDets):
        xo = x[i]
        # Final bin position values corrected for offsets and gain
        x_bin_new.append(xo + (xo * (nDets - xo) * gain_error))

    return x_bin_new


def correct_tube_to_ideal_tube(
    tube_points: List,
    ideal_tube_points: List,
    n_detectors: int,
    test_mode: bool = False,
    polin_fit: int = 2,
    parameters_table: Optional[str] = None,
) -> numpy.ndarray:
    r"""
    Corrects position errors in a tube given an array of points and their ideal positions.

    Note that any element of tubePoints not between 0.0 and nDets is considered a rogue point and so is ignored.

    :param tube_points: Array of Slit Points along tube to be fitted (in pixels)
    :param ideal_tube_points: The corresponding points in an ideal tube (Y-coords advised)
    :param n_detectors: Number of pixel detectors in tube
    :param test_mode: If true, detectors at the position of a slit will be moved out of the way
                      to show the reckoned slit positions when the instrument is displayed.
    :param polin_fit: Order of the polynomial to fit for the ideal positions
    :param parameters_table: name of output TableWorkspace containing values and errors for optimized polynomial
        coefficients, as well as goodness-of-fit chi-square value. If `None`, no table is returned

    :returns: Array of corrected Xs  (in same units as ideal tube points)
    """

    # Check the arguments
    if len(tube_points) != len(ideal_tube_points):
        print("Number of points in tube {0} must equal number of points in ideal tube {1}".format(len(tube_points), len(ideal_tube_points)))
        return x_result

    # Filter out rogue slit points
    used_tube_points = []
    used_ideal_tube_points = []
    missed_tube_points = []  # Used for diagnostic print only
    for i in range(len(tube_points)):
        if 0.0 < tube_points[i] < n_detectors:
            used_tube_points.append(tube_points[i])
            used_ideal_tube_points.append(ideal_tube_points[i])
        else:
            missed_tube_points.append(i + 1)

        # State number of rogue slit points, if any
        print("Only {0} out of {1} slit points used. Missed {2}".format(len(used_tube_points), len(tube_points), missed_tube_points))

    # Check number of usable points
    if len(used_tube_points) < 3:
        print("Too few usable points in tube {0}".format(len(used_tube_points)))
        return []

    # Fit quadratic to ideal tube points
    CreateWorkspace(dataX=used_tube_points, dataY=used_ideal_tube_points, OutputWorkspace="PolyFittingWorkspace")
    try:
        Fit(
            InputWorkspace="PolyFittingWorkspace",
            Function="name=Polynomial,n=%d" % polin_fit,
            StartX=str(0.0),
            EndX=str(n_detectors),
            Output="QF",
        )
    except:
        print("Fit failed")
        return []

    param_q_f = ADS.retrieve("QF_Parameters")

    # get the coefficients, get the Value from every row, and exclude the last one because it is the error
    # rowErr is the last one, it could be used to check accuracy of fit
    c = [r["Value"] for r in param_q_f][:-1]

    # Modify the output array by the fitted quadratic
    x_result = numpy.polynomial.polynomial.polyval(list(range(n_detectors)), c)

    # In test mode, shove the pixels that are closest to the reckoned peaks
    # to the position of the first detector so that the resulting gaps can be seen.
    if test_mode:
        print("TestMode code")
        for i in range(len(used_tube_points)):
            x_result[int(used_tube_points[i])] = x_result[0]

    # Create a copy of the parameters table if the table is requested
    if isinstance(parameters_table, str) and len(parameters_table) > 0:
        CloneWorkspace(InputWorkspace=param_q_f, OutputWorkspace=parameters_table)

    return x_result


def getCalibratedPixelPositions(
    input_workspace: WorkspaceInput,
    tube_positions: ArrayInt,
    ideal_tube_positions: ArrayInt,
    which_tube: ArrayInt,
    peak_test_mode: bool = False,
    polin_fit: int = 2,
    parameters_table: Optional[str] = None,
) -> Tuple[List[int], List[int]]:
    r"""
    Get the calibrated detector positions for one tube.

    The tube is specified by a list of workspace indices of its spectra. The calibration is assumed
    to be done parallel to the Y-axis.

    :param input_workspace: Workspace with tubes to be calibrated - may be integrated or raw
    :param tube_positions: Array of calibration positions (in pixels)
    :param ideal_tube_positions: Where these calibration positions should be (in Y coords)
    :param which_tube:  a list of workspace indices for the tube
    :param peak_test_mode: true if shoving detectors that are reckoned to be at peak away (for test purposes)
    :param polin_fit: Order of the polynomial to fit for the ideal positions
    :param parameters_table: name of output TableWorkspace containing values and errors for optimized polynomial
        coefficients, as well as goodness-of-fit chi-square value. If `None`, no table is returned

    :returns: list of pixel detector IDs, and list of their calibrated positions
    """
    ws = ADS.retrieve(str(input_workspace))  # handle to the workspace
    # Arrays to be returned
    det_IDs = []
    det_positions = []
    # Get position of first and last pixel of tube
    n_dets = len(which_tube)
    if n_dets < 1:
        return det_IDs, det_positions

    # Correct positions of detectors in tube by quadratic fit. If so requested, store the table of fit parameters
    pixels = correct_tube_to_ideal_tube(
        tube_positions, ideal_tube_positions, n_dets, test_mode=peak_test_mode, polin_fit=polin_fit, parameters_table=parameters_table
    )
    if len(pixels) != n_dets:
        print("Tube correction failed.")
        return det_IDs, det_positions
    base_instrument = ws.getInstrument().getBaseInstrument()
    # Get tube unit vector
    # get the detector from the baseInstrument, in order to get the positions
    # before any calibration being loaded.
    det0 = base_instrument.getDetector(ws.getDetector(which_tube[0]).getID())
    detN = base_instrument.getDetector(ws.getDetector(which_tube[-1]).getID())
    d0pos, dNpos = det0.getPos(), detN.getPos()
    # identical to norm of vector: |dNpos - d0pos|
    tubeLength = det0.getDistance(detN)
    if tubeLength <= 0.0:
        print("Zero length tube cannot be calibrated, calibration failed.")
        return det_IDs, det_positions
    # unfortunately, the operation '/' is not defined in V3D object, so
    # I have to use the multiplication.
    # unit_vectors are defined as u = (v2-v1)/|v2-v1| = (dn-d0)/length
    unit_vector = (dNpos - d0pos) * (1.0 / tubeLength)

    # Get Centre (really want to get if from IDF to allow calibration a multiple number of times)
    center = (dNpos + d0pos) * 0.5  # (1.0/2)

    # Move the pixel detectors (might not work for sloping tubes)
    for i in range(n_dets):
        deti = ws.getDetector(which_tube[i])
        p_new = pixels[i]
        # again, the operation float * v3d is not defined, but v3d * float is,
        # so, I wrote the new pos as center + unit_vector * (float)
        new_pos = center + unit_vector * p_new

        det_IDs.append(deti.getID())
        det_positions.append(new_pos)

    return det_IDs, det_positions


def read_peak_file(file_name):
    """Load the file calibration

    It returns a list of tuples, where the first value is the detector identification
    and the second value is its calibration values.

    Example of usage:
        for (det_code, cal_values) in readPeakFile('pathname/TubeDemo'):
            print(det_code)
            print(cal_values)

    """
    loaded_file = []
    # split the entries to the main values:
    # For example:
    # MERLIN/door1/tube_1_1 [34.199347724575574, 525.5864438725401, 1001.7456248836971]
    # Will be splited as:
    # ['MERLIN/door1/tube_1_1', '', '34.199347724575574', '', '525.5864438725401', '', '1001.7456248836971', '', '', '']
    pattern = re.compile(r"[\[\],\s\r]")
    save_directory = config["defaultsave.directory"]
    pfile = os.path.join(save_directory, file_name)
    for line in open(pfile, "r"):
        # check if the entry is a comment line
        if line.startswith("#"):
            continue
        # split all values
        line_vals = re.split(pattern, line)
        id_ = line_vals[0]
        if id_ == "":
            continue
        try:
            f_values = [float(v) for v in line_vals[1:] if v != ""]
        except ValueError:
            continue

        loaded_file.append((id_, f_values))
    return loaded_file


### THESE FUNCTIONS NEXT SHOULD BE THE ONLY FUNCTIONS THE USER CALLS FROM THIS FILE


def getCalibration(
    input_workspace: Union[str, Workspace2D],
    tubeSet: TubeSpec,
    calibTable: TableWorkspace,
    fitPar: TubeCalibFitParams,
    iTube: IdealTube,
    peaksTable: TableWorkspace,
    overridePeaks: Dict[int, List[Any]] = dict(),
    excludeShortTubes: float = 0.0,
    plotTube: List[int] = [],
    range_list: Optional[List[int]] = None,
    polinFit: int = 2,
    peaksTestMode: bool = False,
    parameters_table_group: Optional[str] = None,
) -> None:
    """
    Get the results the calibration and put them in the calibration table provided.

    :param input_workspace: Integrated Workspace with tubes to be calibrated
    :param tubeSet: Specification of Set of tubes to be calibrated ( :class:`~tube_spec.TubeSpec` object)
    :param calibTable: Empty calibration table into which the calibration results are placed. It is composed
        by 'Detector ID' and a V3D column 'Detector Position'. It will be filled with the IDs and calibrated
        positions of the detectors.
    :param fitPar: A :class:`~tube_calib_fit_params.TubeCalibFitParams` object for fitting the peaks
    :param iTube: The :class:`~ideal_tube.IdealTube` which contains the positions in metres of the shadows
        of the slits, bars or edges used for calibration.
    :param peaksTable: Peaks table into which the peaks positions will be put
    :param overridePeaks: dictionary with tube indexes keys and an array of peaks in pixels to override those
        that would be fitted for one tube
    :param excludeShortTubes: Exlude tubes shorter than specified length from calibration
    :param plotTube: List of tube indexes that will be ploted
    :param range_list: list of the tube indexes that will be calibrated. Default None, means all the tubes in tubeSet
    :param polinFit: Order of the polynomial to fit against the known positions. Acceptable: 2, 3
    :param peaksTestMode: true if shoving detectors that are reckoned to be at peak away (for test purposes)
    :param parameters_table_group: name of the WorkspaceGroup containing individual TableWorkspace tables. Each
        table holds values and errors for the optimized coefficients of the polynomial that fits the peak positions (in
        pixel coordinates) to the known slit positions (along the Y-coordinate). The last entry in the table
        holds the goodness-of-fit, chi-square value. The name of each individual TableWorkspace is the string
        `parameters_table_group` plus the suffix `_I`, where `I` is the tube index as given by list `range_list`.
        If `None`, no group workspace is generated.

    This is the main method called from :func:`~tube.calibrate` to perform the calibration.
    """
    ws = ADS.retrieve(str(input_workspace))  # handle to the input workspace
    n_tubes = tubeSet.getNumTubes()
    print("Number of tubes =", n_tubes)

    if range_list is None:
        range_list = range(n_tubes)

    all_skipped = set()

    parameters_tables = list()  # hold the names of all the fit parameter tables
    for i in range_list:
        # Deal with (i+1)st tube specified
        wht, skipped = tubeSet.getTube(i)
        all_skipped.update(skipped)

        print("Calibrating tube", i + 1, "of", n_tubes, tubeSet.getTubeName(i))
        if len(wht) < 1:
            print("Unable to get any workspace indices (spectra) for this tube. Tube", tubeSet.getTubeName(i), "not calibrated.")
            # skip this tube
            continue

        # Calibrate the tube, if possible
        if tubeSet.getTubeLength(i) <= excludeShortTubes:
            # skip this tube
            continue

        ##############################
        # Define Peak Position session
        ##############################

        # if this tube is to be override, get the peaks positions for this tube.
        if i in overridePeaks:
            actual_tube = overridePeaks[i]
        else:
            # find the peaks positions
            plot_this_tube = i in plotTube
            actual_tube = getPoints(ws, iTube.getFunctionalForms(), fitPar, wht, show_plot=plot_this_tube)
            if plot_this_tube:
                RenameWorkspace("FittedData", OutputWorkspace="FittedTube%d" % (i))
                RenameWorkspace("TubePlot", OutputWorkspace="TubePlot%d" % (i))

        # Set the peak positions at the peakTable
        peaksTable.addRow([tubeSet.getTubeName(i)] + list(actual_tube))

        ##########################################
        # Define the correct position of detectors
        ##########################################
        if parameters_table_group is None:
            parameters_table = None
        else:
            parameters_table = f"{parameters_table_group}_{i}"
            parameters_tables.append(parameters_table)
        det_id_list, det_position_list = getCalibratedPixelPositions(
            ws, actual_tube, iTube.getArray(), wht, peaksTestMode, polinFit, parameters_table=parameters_table
        )
        # save the detector positions to calibTable
        if len(det_id_list) == len(wht):  # We have corrected positions
            for j in range(len(wht)):
                next_row = {"Detector ID": det_id_list[j], "Detector Position": det_position_list[j]}
                calibTable.addRow(next_row)

    if len(all_skipped) > 0:
        print("%i histogram(s) were excluded from the calibration since they did not have an assigned detector." % len(all_skipped))

    # Create the WorkspaceGroup containing the fit parameters tables
    if len(parameters_tables) > 0:
        GroupWorkspaces(InputWorkspaces=parameters_tables, OutputWorkspace=parameters_table_group)

    # Delete temporary workspaces used in the calibration
    for ws_name in (
        "TubePlot",
        "CalibPoint_NormalisedCovarianceMatrix",
        "CalibPoint_NormalisedCovarianceMatrix",
        "CalibPoint_NormalisedCovarianceMatrix",
        "CalibPoint_Parameters",
        "CalibPoint_Workspace",
        "PolyFittingWorkspace",
        "QF_NormalisedCovarianceMatrix",
        "QF_Parameters",
        "QF_Workspace",
        "Z1_Workspace",
        "Z1_Parameters",
        "Z1_NormalisedCovarianceMatrix",
    ):
        try:
            DeleteWorkspace(ws_name)
        except:
            pass


def getCalibrationFromPeakFile(ws, calibTable, iTube, PeakFile):
    """
    Get the results the calibration and put them in the calibration table provided.

    @param ws: Integrated Workspace with tubes to be calibrated
    @param calibTable: Calibration table into which the calibration results are placed
    @param  iTube: The ideal tube
    @param PeakFile: File of peaks for calibration

    """

    # Get Ideal Tube
    ideal_tube = iTube.getArray()

    # Read Peak File
    peak_array = read_peak_file(PeakFile)
    n_tubes = len(peak_array)
    print("Number of tubes read from file =", n_tubes)

    for i in range(n_tubes):
        # Deal with (i+1)st tube got from file
        tube_name = peak_array[i][0]  # e.g. 'MERLIN/door3/tube_3_1'
        tube = TubeSpec(ws)
        tube.setTubeSpecByString(tube_name)
        actual_tube = peak_array[i][1]  # e.g.  [2.0, 512.5, 1022.0]

        wht, _ = tube.getTube(0)
        print("Calibrating tube", i + 1, "of", n_tubes, tube_name)
        if len(wht) < 1:
            print("Unable to get any workspace indices for this tube. Calibration abandoned.")
            return

        det_id_list, det_pos_list = getCalibratedPixelPositions(ws, actual_tube, ideal_tube, wht)

        if len(det_id_list) == len(wht):  # We have corrected positions
            for j in range(len(wht)):
                next_row = {"Detector ID": det_id_list[j], "Detector Position": det_pos_list[j]}
                calibTable.addRow(next_row)

    if n_tubes == 0:
        return

    # Delete temporary workspaces for getting new detector positions
    DeleteWorkspace("PolyFittingWorkspace")
    DeleteWorkspace("QF_NormalisedCovarianceMatrix")
    DeleteWorkspace("QF_Parameters")
    DeleteWorkspace("QF_Workspace")


## implement this function
def constructIdealTubeFromRealTube(ws, tube, fitPar, funcForm):
    """
    Construct an ideal tube from an actual tube (assumed ideal)

    :param ws: integrated workspace
    :param tube: specification of one tube (if several tubes, only first tube is used)
    :param fitPar: initial fit parameters for peak of the tube
    :param funcForm: listing the type of known positions 1=Gaussian; 2=edge
    :rtype: IdealTube

    """
    # Get workspace indices
    ideal_tube = IdealTube()

    n_tubes = tube.getNumTubes()
    if n_tubes < 1:
        raise RuntimeError("Invalid tube specification received by constructIdealTubeFromRealTube")
    elif n_tubes > 1:
        print("Specification has several tubes. The ideal tube will be based on the first tube", tube.getTubeName(0))

    wht, _ = tube.getTube(0)

    # Check tube
    if len(wht) < 1:
        raise RuntimeError("Unable to get any workspace indices for this tube. Cannot use as ideal tube.")

        # Get actual tube on which ideal tube is based
    actual_tube = getPoints(ws, funcForm, fitPar, wht)
    print("Actual tube that ideal tube is to be based upon", actual_tube)

    # Get ideal tube based on this actual tube
    try:
        ideal_tube.setArray(actual_tube)
    except:
        msg = "Attempted to create ideal tube based on actual tube" + str(actual_tube)
        msg += "Unable to create ideal tube."
        msg += "Please choose another tube for constructIdealTubeFromRealTube()."
        raise RuntimeError(msg)
    return ideal_tube
