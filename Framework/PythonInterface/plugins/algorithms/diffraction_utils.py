# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2026 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
"""Instrument-agnostic diffraction helpers.

These are shared by POLDI and by the Pawley fitting utilities in scripts/Engineering; they live
here rather than in poldi_utils so that non-POLDI code does not have to import a POLDI module.
"""

from __future__ import annotations
import numpy as np
from typing import Tuple, TYPE_CHECKING

if TYPE_CHECKING:
    from mantid.dataobjects import Workspace2D


def get_dspac_limits(tth_min: float, tth_max: float, lambda_min: float, lambda_max: float) -> Tuple[float, float]:
    """
    Function to calculate min and max d-spacing to consider in auto-correlation given two-theta coverage and wavelength range.
    :param tth_min: minimum two theta to consider
    :param tth_max: maximum two theta to consider
    :param lambda_min: minimum wavelength to consider
    :param lambda_max: maximum wavelength to consider
    :return (dspac_min, dspac_max): min and max d-spacing
    """
    dspac_min = lambda_min / (2 * np.sin(tth_max / 2))
    dspac_max = lambda_max / (2 * np.sin(tth_min / 2))
    return dspac_min, dspac_max


def get_live_spectra(ws: Workspace2D) -> np.ndarray[bool]:
    """
    Function to identify the spectra contributing to a calculation, i.e. those with detectors that aren't masked
    :param ws: MatrixWorkspace containing POLDI data (raw instrument)
    :return np.ndarray: boolean array, True for spectra with detectors that are not masked
    """
    si = ws.spectrumInfo()
    return np.array([si.hasDetectors(ispec) and not si.isMasked(ispec) for ispec in range(ws.getNumberHistograms())])


def get_tths_and_l2s(ws: Workspace2D, is_live: np.ndarray[bool] | None = None) -> Tuple[np.ndarray[float], np.ndarray[float]]:
    """
    Function to get the two-theta and L2 of every spectrum in a workspace
    :param ws: MatrixWorkspace containing POLDI data (raw instrument)
    :param is_live: boolean array, True for spectra that are not masked (calculated from ws if not provided)
    :return (tths, l2s): two-theta (rad) and sample to detector distance (m) of each spectrum - masked spectra are
                         set to zero rather than removed, so the arrays can still be indexed by workspace index
    """
    if is_live is None:
        is_live = get_live_spectra(ws)
    si = ws.spectrumInfo()
    tths = np.array([si.twoTheta(ispec) if live_spec else 0.0 for ispec, live_spec in enumerate(is_live)])
    l2s = np.array([si.l2(ispec) if live_spec else 0.0 for ispec, live_spec in enumerate(is_live)])
    return tths, l2s


def get_live_tths_and_l2s(ws: Workspace2D) -> Tuple[np.ndarray[float], np.ndarray[float]]:
    """
    Function to get the two-theta and L2 of the unmasked spectra in a workspace
    :param ws: MatrixWorkspace containing POLDI data (raw instrument)
    :return (tths, l2s): two-theta (rad) and sample to detector distance (m) of each unmasked spectrum
    """
    is_live = get_live_spectra(ws)
    tths, l2s = get_tths_and_l2s(ws, is_live)
    return tths[is_live], l2s[is_live]


def get_dspac_limits_from_ws(ws: Workspace2D, lambda_min: float = 1.1, lambda_max: float = 5.0) -> Tuple[float, float]:
    """
    Function to calculate min and max d-spacing accessible in a workspace given its two-theta coverage
    :param ws: MatrixWorkspace containing POLDI data (raw instrument)
    :param lambda_min: minimum wavelength (Ang) to consider
    :param lambda_max: maximum wavelength (Ang) to consider
    :return (dspac_min, dspac_max): min and max d-spacing
    """
    tths, _ = get_live_tths_and_l2s(ws)
    return get_dspac_limits(tths.min(), tths.max(), lambda_min, lambda_max)
