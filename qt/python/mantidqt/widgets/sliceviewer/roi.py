# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantidqt.
#
import math

from mantid.api import MatrixWorkspace
from mantid.simpleapi import (DeleteWorkspace, ExtractSpectra, Rebin, ReplaceSpecialValues,
                              SumSpectra, Transpose)
import numpy as np


def extract_matrix_cuts_spectra_axis(workspace: MatrixWorkspace,
                                     xmin: float,
                                     xmax: float,
                                     ymin: float,
                                     ymax: float,
                                     xcut_name: str,
                                     ycut_name: str,
                                     log_algorithm_calls: bool = False):
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
    tmp_crop_region = '__tmp_sv_region_extract'
    roi = extract_roi_spectra_axis(workspace, xmin, xmax, ymin, ymax, tmp_crop_region,
                                   log_algorithm_calls)
    # perform ycut first so xcut can reuse tmp workspace for rebinning if necessary
    if ycut_name:
        Rebin(
            InputWorkspace=roi,
            OutputWorkspace=ycut_name,
            Params=[xmin, xmax - xmin, xmax],
            EnableLogging=log_algorithm_calls)
        Transpose(
            InputWorkspace=ycut_name, OutputWorkspace=ycut_name, EnableLogging=log_algorithm_calls)

    if xcut_name:
        if not roi.isCommonBins():
            # rebin to a common grid using the resolution from the spectrum
            # with the lowest resolution to avoid overbinning
            roi = _rebin_to_common_grid(roi, xmin, xmax, log_algorithm_calls)
        SumSpectra(InputWorkspace=roi, OutputWorkspace=xcut_name, EnableLogging=log_algorithm_calls)

    try:
        DeleteWorkspace(tmp_crop_region)
    except ValueError:
        pass


def extract_matrix_cuts_numeric_axis(workspace: MatrixWorkspace,
                                     xmin: float,
                                     xmax: float,
                                     ymin: float,
                                     ymax: float,
                                     xcut_name: str,
                                     ycut_name: str,
                                     log_algorithm_calls: bool = False):
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
    if xcut_name:
        ExtractSpectra(
            InputWorkspace=workspace,
            OutputWorkspace=xcut_name,
            XMin=xmin,
            XMax=xmax,
            EnableLogging=log_algorithm_calls)
        # Rebin with nan/inf results in everything turning to NAN so
        # set them all to zero as this will effectively ignore them for us
        ReplaceSpecialValues(
            InputWorkspace=xcut_name,
            OutputWorkspace=xcut_name,
            NanValue=0.0,
            InfinityValue=0.0,
            EnableLogging=log_algorithm_calls)
        Transpose(
            InputWorkspace=xcut_name, OutputWorkspace=xcut_name, EnableLogging=log_algorithm_calls)
        Rebin(
            InputWorkspace=xcut_name,
            OutputWorkspace=xcut_name,
            Params=[ymin, ymax - ymin, ymax],
            EnableLogging=log_algorithm_calls)
        Transpose(
            InputWorkspace=xcut_name, OutputWorkspace=xcut_name, EnableLogging=log_algorithm_calls)
    if ycut_name:
        Rebin(
            InputWorkspace=workspace,
            OutputWorkspace=ycut_name,
            Params=[xmin, xmax - xmin, xmax],
            EnableLogging=log_algorithm_calls)
        Transpose(
            InputWorkspace=ycut_name, OutputWorkspace=ycut_name, EnableLogging=log_algorithm_calls)


def extract_roi_matrix(workspace: MatrixWorkspace,
                       xmin: float,
                       xmax: float,
                       ymin: float,
                       ymax: float,
                       transpose: bool,
                       roi_name: str,
                       log_algorithm_calls: bool = False):
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
    if transpose:
        xmin, xmax, ymin, ymax = ymin, ymax, xmin, xmax

    yaxis = workspace.getAxis(1)
    if yaxis.isSpectra():
        extract_roi_spectra_axis(workspace, xmin, xmax, ymin, ymax, roi_name, log_algorithm_calls)
    elif yaxis.isNumeric():
        extract_roi_numeric_axis(workspace, xmin, xmax, ymin, ymax, roi_name, log_algorithm_calls)
    else:
        raise RuntimeError("Unknown vertical axis type")

    if transpose:
        Transpose(InputWorkspace=roi_name, OutputWorkspace=roi_name)


def extract_roi_spectra_axis(workspace: MatrixWorkspace,
                             xmin: float,
                             xmax: float,
                             ymin: float,
                             ymax: float,
                             roi_name: str,
                             log_algorithm_calls: bool = False):
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
    return _extract_region(workspace, xmin, xmax, indexmin, indexmax, roi_name, log_algorithm_calls)


def extract_roi_numeric_axis(workspace: MatrixWorkspace,
                             xmin: float,
                             xmax: float,
                             ymin: float,
                             ymax: float,
                             roi_name: str,
                             log_algorithm_calls: bool = False):
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
    yaxis_values = workspace.getAxis(1).extractValues()
    # determine workspace indices for crop. We assume the region has been checked against the workspace
    # data boundaries
    indexmin, indexmax = int(np.searchsorted(yaxis_values, ymin)), \
        int(np.searchsorted(yaxis_values, ymax) - 1)
    return _extract_region(workspace, xmin, xmax, indexmin, indexmax, roi_name, log_algorithm_calls)


def _extract_region(workspace: MatrixWorkspace,
                    xmin: float,
                    xmax: float,
                    indexmin: int,
                    indexmax: int,
                    name: str,
                    log_algorithm_calls: bool = False):
    """
    Assuming a MatrixWorkspace crop a region with the given parameters
    :param workspace: A MatrixWorkspace with a vertical SpectraAxis
    :param xmin: X min for bounded region
    :param xmax: X max for bounded region
    :param ymin: Y min for bounded region
    :param ymax: Y max for bounded region
    :param name: A name for the workspace
    """
    return ExtractSpectra(
        InputWorkspace=workspace,
        OutputWorkspace=name,
        XMin=xmin,
        XMax=xmax,
        StartWorkspaceIndex=indexmin,
        EndWorkspaceIndex=indexmax,
        EnableLogging=log_algorithm_calls)


def _index_range_spectraaxis(workspace: MatrixWorkspace, ymin: float, ymax: float):
    """
    Return the workspace indicies for the given ymin/ymax values on the given workspace
    :param workspace: A MatrixWorkspace object
    :param ymin: Minimum Y value in range
    :param ymax: Maximum Y value in range
    """
    indexmin = workspace.getIndexFromSpectrumNumber(math.floor(ymin + 0.5))
    indexmax = workspace.getIndexFromSpectrumNumber(math.floor(ymax + 0.5))
    return indexmin, indexmax


def _rebin_to_common_grid(workspace: MatrixWorkspace, xmin: float, xmax: float,
                          log_algorithm_calls: bool):
    """
    Assuming the workspace is ragged, rebin it to a common grid with the range
    given. The resolution is computed by taking the largest bin size in the
    region.
    :param workspace: A MatrixWorkspace object
    :param xmin: Minimum X value in range
    :param xmax: Maximum X value in range
    """
    delta = np.max(np.diff(workspace.extractX()))
    return Rebin(
        InputWorkspace=workspace,
        OutputWorkspace=workspace,
        Params=[xmin, delta, xmax],
        EnableLogging=log_algorithm_calls)
