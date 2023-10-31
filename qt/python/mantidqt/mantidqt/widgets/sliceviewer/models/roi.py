# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantidqt.
#
from mantid.api import AnalysisDataService, MatrixWorkspace
from mantid.simpleapi import DeleteWorkspace, ExtractSpectra, Rebin, SumSpectra, Transpose
import numpy as np
from mantid.dataobjects import EventWorkspace


def on_except_leave_ads_unchanged(func):
    """
    If the decorated function raises and exception the remove any workspaces
    from from the ADS that were added during the wrapped function call.
    The exception raised in the wrapper is reraised.
    """

    def wrapper(*args, **kwargs):
        ads = AnalysisDataService.Instance()
        names_before = set(ads.getObjectNames())
        try:
            return func(*args, **kwargs)
        except Exception as exc:
            added_names = set(ads.getObjectNames()) - names_before
            for name in added_names:
                ads.remove(name)
            raise exc

    return wrapper


@on_except_leave_ads_unchanged
def extract_matrix_cuts_spectra_axis(
    workspace: MatrixWorkspace,
    xmin: float,
    xmax: float,
    ymin: float,
    ymax: float,
    xcut_name: str,
    ycut_name: str,
    log_algorithm_calls: bool = True,
):
    """
    Assuming a MatrixWorkspace with vertical spectra axis, extract 1D cuts from the region defined
    by the given parameters
    :param workspace: A MatrixWorkspace with a vertical SpectraAxis
    :param xmin: X min for bounded region
    :param xmax: X max for bounded region
    :param ymin: Y min for bounded region
    :param ymax: Y max for bounded region
    :param xcut_name: Name of the X cut. Empty indicates it should be skipped
    :param ycut_name: Name of the Y cut. Empty indicates it should be skipped
    :param log_algorithm_calls: Log the algorithm call or be silent
    """
    # if the value is half way in a spectrum then include it
    tmp_crop_region = "__tmp_sv_region_extract"
    roi = _extract_roi_spectra_axis(workspace, xmin, xmax, ymin, ymax, tmp_crop_region, log_algorithm_calls)
    # perform ycut first so xcut can reuse tmp workspace for rebinning if necessary
    if ycut_name:
        Rebin(
            InputWorkspace=roi,
            OutputWorkspace=ycut_name,
            Params=[xmin, xmax - xmin, xmax],
            EnableLogging=log_algorithm_calls,
            PreserveEvents=False,
        )
        Transpose(InputWorkspace=ycut_name, OutputWorkspace=ycut_name, EnableLogging=log_algorithm_calls)

    if xcut_name:
        if not roi.isCommonBins():
            # rebin to a common grid using the resolution from the spectrum
            # with the lowest resolution to avoid overbinning
            roi = _rebin_to_common_grid(roi, xmin, xmax, log_algorithm_calls)
        SumSpectra(InputWorkspace=roi, OutputWorkspace=xcut_name, EnableLogging=log_algorithm_calls)

    try:
        DeleteWorkspace(tmp_crop_region, EnableLogging=log_algorithm_calls)
    except ValueError:
        pass


@on_except_leave_ads_unchanged
def extract_cuts_matrix(
    workspace: MatrixWorkspace,
    xmin: float,
    xmax: float,
    ymin: float,
    ymax: float,
    xcut_name: str,
    ycut_name: str,
    log_algorithm_calls: bool = True,
):
    """
    Assuming a MatrixWorkspace with vertical numeric axis, extract 1D cuts from the region defined
    by the given parameters
    :param workspace: A MatrixWorkspace with a vertical NumericAxis
    :param xmin: X min for bounded region
    :param xmax: X max for bounded region
    :param ymin: Y min for bounded region
    :param ymax: Y max for bounded region
    :param xcut_name: Name of the X cut. Empty indicates it should be skipped
    :param ycut_name: Name of the Y cut. Empty indicates it should be skipped
    :param log_algorithm_calls: Log the algorithm call or be silent
    """
    tmp_crop_region = "__tmp_sv_region_extract"
    transpose = False
    roi = extract_roi_matrix(workspace, xmin, xmax, ymin, ymax, transpose, tmp_crop_region, log_algorithm_calls)

    # perform ycut first so xcut can reuse tmp workspace for rebinning if necessary
    if ycut_name:
        Rebin(
            InputWorkspace=roi,
            OutputWorkspace=ycut_name,
            Params=[xmin, xmax - xmin, xmax],
            EnableLogging=log_algorithm_calls,
            PreserveEvents=False,
        )
        Transpose(InputWorkspace=ycut_name, OutputWorkspace=ycut_name, EnableLogging=log_algorithm_calls)

    if xcut_name:
        if not roi.isCommonBins():
            # rebin to a common grid using the resolution from the spectrum
            # with the lowest resolution to avoid overbinning
            roi = _rebin_to_common_grid(roi, xmin, xmax, log_algorithm_calls)
        SumSpectra(InputWorkspace=roi, OutputWorkspace=xcut_name, EnableLogging=log_algorithm_calls)

    try:
        DeleteWorkspace(tmp_crop_region, EnableLogging=log_algorithm_calls)
    except ValueError:
        pass


@on_except_leave_ads_unchanged
def extract_roi_matrix(
    workspace: MatrixWorkspace,
    xmin: float,
    xmax: float,
    ymin: float,
    ymax: float,
    transpose: bool,
    roi_name: str,
    log_algorithm_calls: bool = True,
):
    """
    Assuming a MatrixWorkspace extract a 2D region from it.
    :param workspace: A MatrixWorkspace
    :param xmin: X min for bounded region
    :param xmax: X max for bounded region
    :param ymin: Y min for bounded region
    :param ymax: Y max for bounded region
    :param transpose: If true then the region bounds are transposed from the orientation of the workspace
    :param roi_name: Name of the result workspace
    :param log_algorithm_calls: Log the algorithm call or be silent
    """
    yaxis = workspace.getAxis(1)
    if yaxis.isSpectra():
        roi = _extract_roi_spectra_axis(workspace, xmin, xmax, ymin, ymax, roi_name, log_algorithm_calls)
    elif yaxis.isNumeric():
        roi = _extract_roi_numeric_axis(workspace, xmin, xmax, ymin, ymax, roi_name, log_algorithm_calls)
    else:
        raise RuntimeError("Unknown vertical axis type")

    if transpose:
        if isinstance(roi, EventWorkspace):
            Rebin(
                InputWorkspace=roi_name,
                OutputWorkspace=roi_name,
                Params=[roi.getTofMin(), roi.getTofMax() - roi.getTofMin(), roi.getTofMax()],
                EnableLogging=log_algorithm_calls,
                PreserveEvents=False,
            )
        roi = Transpose(InputWorkspace=roi_name, OutputWorkspace=roi_name)

    return roi


# private api


def _extract_roi_spectra_axis(
    workspace: MatrixWorkspace, xmin: float, xmax: float, ymin: float, ymax: float, roi_name: str, log_algorithm_calls: bool = True
):
    """
    Assuming a MatrixWorkspace with vertical spectra axis, extract 1D cuts from the region defined
    by the given parameters
    :param workspace: A MatrixWorkspace with a vertical SpectraAxis
    :param xmin: X min for bounded region
    :param xmax: X max for bounded region
    :param ymin: Y min for bounded region
    :param ymax: Y max for bounded region
    :param xcut_name: Name of the X cut. Empty indicates it should be skipped
    :param ycut_name: Name of the Y cut. Empty indicates it should be skipped
    :param log_algorithm_calls: Log the algorithm call or be silent
    """
    indexmin, indexmax = _index_range_spectraaxis(workspace, ymin, ymax)
    xmin, xmax = _adjust_xmin_xmax_for_event_workspace(workspace, xmin, xmax, indexmin)
    return _extract_region(workspace, xmin, xmax, indexmin, indexmax, roi_name, log_algorithm_calls)


def _extract_roi_numeric_axis(
    workspace: MatrixWorkspace, xmin: float, xmax: float, ymin: float, ymax: float, roi_name: str, log_algorithm_calls: bool = True
):
    """
    Assuming a MatrixWorkspace with vertical spectra axis, extract 1D cuts from the region defined
    by the given parameters
    :param workspace: A MatrixWorkspace with a vertical SpectraAxis
    :param xmin: X min for bounded region. Can be None indicating use data xmin
    :param xmax: X max for bounded region. Can be None indicating use data xmax
    :param ymin: Y min for bounded region.
    :param ymax: Y max for bounded region.
    :param xcut_name: Name of the X cut. Empty indicates it should be skipped
    :param ycut_name: Name of the Y cut. Empty indicates it should be skipped
    :param log_algorithm_calls: Log the algorithm call or be silent
    """
    indexmin, indexmax = _index_range_numericaxis(workspace, ymin, ymax)
    return _extract_region(workspace, xmin, xmax, indexmin, indexmax, roi_name, log_algorithm_calls)


def _adjust_xmin_xmax_for_event_workspace(workspace, xmin, xmax, ws_index_min):
    """ "
    This method adjusts the xmin and xmax values to the edges of the corresponding bin for EventWorkspaces when xmin and xmax are equal
    :param workspace: A MatrixWorkspace with a vertical SpectraAxis
    :param xmin: X min for bounded region. Can be None indicating use data xmin
    :param xmax: X max for bounded region. Can be None indicating use data xmax
    :param ws_index_min: minimum Y index for bounded region
    """
    if not isinstance(workspace, EventWorkspace):
        return xmin, xmax

    if xmin == xmax and xmin is not None:
        x_data = workspace.readX(ws_index_min)
        bin_index_to_xmin = workspace.yIndexOfX(xmin, ws_index_min)
        return x_data[bin_index_to_xmin], x_data[bin_index_to_xmin + 1]
    else:
        return xmin, xmax


def _extract_region(
    workspace: MatrixWorkspace, xmin: float, xmax: float, indexmin: int, indexmax: int, name: str, log_algorithm_calls: bool = True
):
    """
    Assuming a MatrixWorkspace crop a region with the given parameters
    :param workspace: A MatrixWorkspace with a vertical SpectraAxis
    :param xmin: X min for bounded region
    :param xmax: X max for bounded region
    :param indexmin: minimum Y index for bounded region
    :param indexmax: maximum Y index for bounded region
    :param name: A name for the workspace
    """
    if not workspace.isCommonBins():
        # rebin to a common grid using the resolution from the spectrum
        # with the lowest resolution to avoid overbinning
        workspace = _rebin_to_common_grid(workspace, None, None, log_algorithm_calls)
    return ExtractSpectra(
        InputWorkspace=workspace,
        OutputWorkspace=name,
        XMin=xmin,
        XMax=xmax,
        StartWorkspaceIndex=indexmin,
        EndWorkspaceIndex=indexmax,
        EnableLogging=log_algorithm_calls,
    )


def _index_range_spectraaxis(workspace: MatrixWorkspace, ymin: float, ymax: float):
    """
    Return the workspace indicies for the given ymin/ymax values on the given workspace
    :param workspace: A MatrixWorkspace object spectra Y Axis
    :param ymin: Minimum Y value in range
    :param ymax: Maximum Y value in range
    """
    if ymin is None or ymax is None:
        return 0, workspace.getNumberHistograms() - 1
    else:
        spectra_axis = workspace.getAxis(1)
        return spectra_axis.indexOfValue(ymin), spectra_axis.indexOfValue(ymax)


def _index_range_numericaxis(workspace: MatrixWorkspace, ymin: float, ymax: float):
    """
    Return the workspace indices for the given ymin/ymax values on the given workspace
    :param workspace: A MatrixWorkspace object with a numeric Y Axis
    :param ymin: Minimum Y value in range
    :param ymax: Maximum Y value in range
    """
    if ymin is None or ymax is None:
        return 0, workspace.getNumberHistograms() - 1
    else:
        yaxis_values = workspace.getAxis(1).extractValues()
        return int(np.searchsorted(yaxis_values, ymin)), int(np.searchsorted(yaxis_values, ymax))


def _rebin_to_common_grid(workspace: MatrixWorkspace, xmin: float, xmax: float, log_algorithm_calls: bool):
    """
    Assuming the workspace is ragged, rebin it to a common grid with the range
    given. The resolution is computed by taking the largest bin size in the
    region.
    :param workspace: A MatrixWorkspace object
    :param xmin: Minimum X value in range
    :param xmax: Maximum X value in range
    """
    delta = np.max(np.diff(workspace.extractX()))
    if xmin is None or xmax is None:
        params = [delta]
    else:
        params = [xmin, delta, xmax]
    return Rebin(InputWorkspace=workspace, OutputWorkspace=workspace, Params=params, EnableLogging=log_algorithm_calls, PreserveEvent=False)
