# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2025 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
from mantid.api import (
    AlgorithmManager,
    FunctionFactory,
)
import numpy as np
from scipy.stats import moment
from mantid.geometry import RectangularDetector, GridDetector
import re

"""
This module contains common utility classes and methods used in peak integration algorithms
"""


class InstrumentArrayConverter:
    """
    This class is used to convert the spectra in the region of a peak to a 3D numpy array of signal and errors
    Note that for RectangularDetectors and GridDetectors it assumes that banks are not adjacent
    - i.e. it truncates the data to the edge of the bank.  For general ComponentArray type detector banks it does
     search for banks with adjacent column-components (on WISH this corresponds to tubes).
    """

    def __init__(self, ws):
        self.ws = ws
        self.inst = ws.getInstrument()
        if any(
            self.inst[icomp]
            for icomp in range(self.inst.nelements())
            if isinstance(self.inst[icomp], RectangularDetector) or isinstance(self.inst[icomp], GridDetector)
        ):
            # might not be true for all components due to presence of monitors etc.
            self.get_detid_array = self._get_detid_array_rect_detector
        else:
            self.get_detid_array = self._get_detid_array_comp_assembly

    @staticmethod
    def split_string_trailing_int(name):
        *_, match = re.finditer(r"\d+", name)  # last match
        return name[: match.start()], name[match.start() :]  # prefix, num_str

    @staticmethod
    def find_nearest_child_to_component(parent, comp, excl=None, ndepth=1):
        dist = np.inf
        nearest_child = None
        for ichild in range(parent.nelements()):
            child = parent[ichild]
            if hasattr(child, "nelements"):
                while child.nelements() == 1:
                    child = child[0]
            is_correct_type = "CompAssembly" in child.type() or child.type() == "DetectorComponent"
            is_not_excluded = excl is None or child.getFullName() != excl
            if is_correct_type and is_not_excluded:
                this_dist = child.getDistance(comp)
                if this_dist < dist:
                    dist = this_dist
                    nearest_child = child
        if ndepth > 1:
            nearest_child = InstrumentArrayConverter.find_nearest_child_to_component(nearest_child, comp, ndepth=ndepth - 1)
        return nearest_child

    def _find_nearest_adjacent_col_component(self, dcol, col, ncols, bank_name, col_prefix, col_str, delim, ncols_edge):
        next_col_prefix, next_col_str = None, None
        isLHS = np.any(dcol + col <= ncols_edge)
        isRHS = np.any(dcol + col >= ncols - ncols_edge + 1)
        if isLHS or isRHS:
            # get tube/col at each end of detector to get tube separation
            detLHS = self.inst.getComponentByName(delim.join([bank_name, f"{col_prefix}{1:0{len(col_str)}d}"]))
            detRHS = self.inst.getComponentByName(delim.join([bank_name, f"{col_prefix}{ncols:0{len(col_str)}d}"]))
            det_sep = detLHS.getDistance(detRHS) / (ncols - 1)  # loose half pix at each end (cen-cen dist.)
            # look for adjacent tube in adjacent banks
            det_ref = detLHS if isLHS else detRHS
            next_det = self.find_nearest_child_to_component(self.inst, det_ref, excl=bank_name, ndepth=2)
            if next_det is not None and next_det.getDistance(det_ref) < 1.1 * det_sep:
                # is considered adjacent
                next_col_prefix, next_col_str = self.split_string_trailing_int(next_det.getFullName())
        return next_col_prefix, next_col_str, isLHS, isRHS

    def _get_detid_array_comp_assembly(self, bank, detid, row, col, drows, dcols, nrows_edge, ncols_edge=1):
        ispec = self.ws.getIndicesFromDetectorIDs([detid])
        ndet_per_spec = len(self.ws.getSpectrum(ispec[0]).getDetectorIDs())
        nrows = bank[0].nelements()
        ncols = bank.nelements()

        # get range of row/col
        drow_vec = np.arange(-drows, drows + 1) * ndet_per_spec  # to account for n-1 detector mapping in a tube
        drow_vec = drow_vec[np.logical_and(drow_vec > -row, drow_vec <= nrows - row)]
        dcol_vec = np.arange(-dcols, dcols + 1)  # can't crop this a-priori as don't know if adjacent bank
        dcol, drow = np.meshgrid(dcol_vec, drow_vec)

        # get row and col component names from string representation of instrument tree
        ci = self.ws.componentInfo()
        det_idx = self.ws.detectorInfo().indexOf(detid)
        row_name = ci.name(det_idx)  # e.g. 'pixel0066'
        row_prefix, row_str = self.split_string_trailing_int(row_name)  # e.g. 'pixel', '0066'
        col_name = ci.name(ci.parent(det_idx))
        col_prefix, col_str = self.split_string_trailing_int(col_name)
        bank_name = bank.getFullName()
        bank_prefix, bank_str = self.split_string_trailing_int(bank_name)  # 'e.g. WISH/panel09/WISHPanel', '09'
        delim = bank_prefix[len(ci.name(ci.root()))]

        # check for adjacent tube in adjacent banks
        next_col_prefix, next_col_str, isLHS, isRHS = self._find_nearest_adjacent_col_component(
            dcol, col, ncols, bank_name, col_prefix, col_str, delim, ncols_edge
        )

        # loop over row/cols and get detids
        detids = np.zeros(dcol.shape, int)
        for icol in range(dcol.shape[1]):
            new_col = col + dcol[0, icol]
            col_comp_name = None  # None indicates no column component (tube) found in this or adjacent bank
            if 0 < new_col <= ncols:  # indexing starts from 1
                # column comp in this bank
                col_comp_name = delim.join([bank_name, f"{col_prefix}{new_col:0{len(col_str)}d}"])
            elif next_col_str is not None:
                # adjacent column component found in another bank
                new_col = int(next_col_str) + new_col
                if isRHS:
                    new_col -= ncols + 1
                col_comp_name = f"{next_col_prefix}{new_col:0{len(next_col_str)}d}"
            for irow in range(dcol.shape[0]):
                if col_comp_name is not None:
                    new_row = row + drow[irow, 0]
                    row_comp_name = f"{row_prefix}{new_row:0{len(row_str)}d}"
                    det = self.inst.getComponentByName(delim.join([col_comp_name, row_comp_name]))
                    if det is not None:
                        detids[irow, icol] = det.getID()

        # remove starting or trailing zeros from detids etc. where no adjacent tubes found in other banks
        icols_keep = detids[0, :] != 0
        detids = detids[:, icols_keep]
        dcol = dcol[:, icols_keep]
        drow = drow[:, icols_keep]
        col_edges = np.zeros(detids.shape, dtype=bool)
        if next_col_str is None:
            # no adjacent bank found (or window not within ncols_edge of detector edge)
            if isLHS and dcol[0, 0] <= ncols_edge - col:
                # window within ncols_edge of detector edge on LHS of the bank with no adjacent tube in another
                col_edges = dcol <= ncols_edge - col
            elif isRHS and dcol[0, -1] > ncols - col - ncols_edge:
                # window within ncols_edge of detector edge on RHS of the bank with no adjacent tube in another
                col_edges = dcol > ncols - col - ncols_edge
        row_edges = np.logical_or(drow <= ndet_per_spec * nrows_edge - row, drow > nrows - row - ndet_per_spec * nrows_edge)
        detector_edges = np.logical_or(row_edges, col_edges)
        # row_peak, icol_peak
        irow_peak = np.where(drow[:, 0] == 0)[0][0]
        icol_peak = np.where(dcol[0, :] == 0)[0][0]
        return detids, detector_edges, irow_peak, icol_peak

    def _get_detid_array_rect_detector(self, bank, detid, row, col, drows, dcols, nrows_edge, ncols_edge):
        col_step, row_step = bank.idstep(), bank.idstepbyrow()  # step in detID along col and row
        # need to adjust range depending on whether above min/max row/col
        drow_vec = np.arange(max(0, row - drows), min(row + drows + 1, bank.xpixels())) - row
        dcol_vec = np.arange(max(0, col - dcols), min(col + dcols + 1, bank.ypixels())) - col
        dcol, drow = np.meshgrid(dcol_vec, drow_vec)
        if bank.idfillbyfirst_y():
            col_step, row_step = row_step, col_step
        detids = detid + dcol * col_step + drow * row_step
        # create bool mask for detector edges
        detector_edges = np.logical_or.reduce(
            (
                drow <= -row + nrows_edge - 1,
                dcol <= -col + ncols_edge - 1,
                drow >= bank.xpixels() - nrows_edge - row,
                dcol >= bank.ypixels() - ncols_edge - col,
            )
        )
        # get indices of peak centre
        irow_peak = np.where(drow_vec == 0)[0][0]
        icol_peak = np.where(dcol_vec == 0)[0][0]
        return detids, detector_edges, irow_peak, icol_peak

    def get_peak_data(self, peak, detid, bank_name, nrows, ncols, nrows_edge, ncols_edge):
        """
        :param peak: peak object
        :param detid: detector id of peak (from peak table)
        :param bank_name: bank name on which detector resides (from peak table)
        :param dpixel: width of detector window in pixels (along row and columns)
        :return peak_data: PeakData object storing detids in peak region
        """
        bank = self.inst.getComponentByName(bank_name)
        row, col = peak.getRow(), peak.getCol()
        drows, dcols = int(nrows) // 2, int(ncols) // 2
        detids, det_edges, irow_peak, icol_peak = self.get_detid_array(bank, detid, row, col, drows, dcols, nrows_edge, ncols_edge)
        # add masked pixels to edges (sometimes used to denote edges or broken components)
        det_info = self.ws.detectorInfo()
        for irow in range(det_edges.shape[0]):
            for icol in range(det_edges.shape[1]):
                if det_info.isMasked(det_info.indexOf(int(detids[irow, icol]))):
                    det_edges[irow, icol] = True
        return self._create_peakdata_instance(irow_peak, icol_peak, det_edges, detids, peak)

    def _create_peakdata_instance(self, irow_peak, icol_peak, det_edges, detids, peak):
        return PeakData(irow_peak, icol_peak, det_edges, detids, peak, self.ws)


class PeakData:
    """
    This class is used to hold data and integration parameters for single-crystal Bragg peaks
    """

    def __init__(self, irow, icol, det_edges, detids, peak, ws):
        # set data
        self.ws = ws
        self.irow, self.icol = irow, icol
        self.det_edges = det_edges
        self.detids = detids

        # extract peak properties
        self.hkl = np.round(peak.getHKL(), 2)  # used for plot title
        self.wl = peak.getWavelength()
        self.theta = peak.getScattering() / 2
        if self.ws.getAxis(0).getUnit().unitID() == "dSpacing":
            self.xpos = peak.getDSpacing()
        else:
            self.xpos = peak.getTOF()

    def get_data_arrays(self):
        # get signal and error from each spectrum
        ispecs = np.array(self.ws.getIndicesFromDetectorIDs([int(d) for d in self.detids.flatten()])).reshape(self.detids.shape)
        signal = np.zeros((*ispecs.shape, self.ws.blocksize()))
        errors = np.zeros(signal.shape)
        xcens = np.zeros(signal.shape)
        for irow in range(signal.shape[0]):
            for icol in range(signal.shape[1]):
                ispec = int(ispecs[irow, icol])
                signal[irow, icol, :] = self.ws.readY(ispec)
                errors[irow, icol, :] = self.ws.readE(ispec)
                xvals = self.ws.readX(ispec)
                if len(xvals) > signal.shape[-1]:
                    xvals = 0.5 * (xvals[:-1] + xvals[1:])  # convert to bin centers
                xcens[irow, icol, :] = xvals
        return xcens, signal, errors**2, ispecs

    @staticmethod
    def find_bg_pts_seed_skew(signal, ibg_seed=None):
        if ibg_seed is None:
            ibg_seed = np.arange(signal.size)
        # sort and grow seed
        isort = np.argsort(-signal[ibg_seed])  # descending order
        istart = 0
        iend = len(signal)
        prev_skew = moment(signal[ibg_seed[isort[istart:iend]]], 3)
        for istart in range(1, iend):
            this_skew = moment(signal[ibg_seed[isort[istart:iend]]], 3)
            if this_skew >= prev_skew or this_skew < 0:
                istart -= 1
                break
            else:
                prev_skew = this_skew
        return ibg_seed[isort[istart:iend]], ibg_seed[isort[:istart]]  # bg, non-bg


def exec_simpleapi_alg(alg_name, **kwargs):
    alg = AlgorithmManager.create(alg_name)
    alg.initialize()
    alg.setLogging(False)
    for prop, value in kwargs.items():
        alg.setProperty(prop, value)
    alg.execute()
    if "OutputWorkspace" in alg.outputProperties():
        return alg.getPropertyValue("OutputWorkspace")
    else:
        return None


def get_fwhm_from_back_to_back_params(pk, ws, detid):
    func = FunctionFactory.Instance().createPeakFunction("BackToBackExponential")
    func.setParameter("X0", pk.getTOF())  # set centre
    func.setMatrixWorkspace(ws, ws.getIndicesFromDetectorIDs([int(detid)])[0], 0.0, 0.0)  # calc A,B,S based on peak cen
    is_valid = all(func.isExplicitlySet(ipar) for ipar in [1, 2, 4])
    return func.fwhm() if is_valid else None


def round_up_to_odd_number(number):
    if not number % 2:
        number += 1
    return number


def get_bin_width_at_tof(ws, ispec, tof):
    itof = ws.yIndexOfX(tof, ispec)
    return ws.readX(ispec)[itof + 1] - ws.readX(ispec)[itof]


def set_peak_intensity(pk, intens, sigma, do_lorz_cor):
    if do_lorz_cor:
        L = (np.sin(pk.getScattering() / 2) ** 2) / (pk.getWavelength() ** 4)  # at updated peak pos
    else:
        L = 1
    # set peak object intensity
    pk.setIntensity(L * intens)
    pk.setSigmaIntensity(L * sigma)


def make_kernel(nrows, ncols, nbins):
    # make a kernel with background subtraction shell of approx. same number of elements as peak region
    kernel_shape, (nrows_bg, ncols_bg, nbins_bg) = get_kernel_shape(nrows, ncols, nbins)
    kernel = np.zeros(kernel_shape)
    kernel[nrows_bg:-nrows_bg, ncols_bg:-ncols_bg, nbins_bg:-nbins_bg] = 1
    bg_mask = np.logical_not(kernel)
    kernel[bg_mask] = -(nrows * ncols * nbins) / bg_mask.sum()  # such that kernel.sum() = 0
    return kernel


def get_kernel_shape(nrows, ncols, nbins):
    nrows_bg = max(1, nrows // 16)  # padding either side of e.g. nrows for bg shell
    ncols_bg = max(1, ncols // 16)
    nbins_bg = max(1, nbins // 16)
    return (nrows + 2 * nrows_bg, ncols + 2 * ncols_bg, nbins + 2 * nbins_bg), (nrows_bg, ncols_bg, nbins_bg)
