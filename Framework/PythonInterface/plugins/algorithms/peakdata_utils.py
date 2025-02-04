# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2025 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
from mantid.api import (
    AlgorithmManager,
    FunctionFactory,
    AnalysisDataService,
)
import numpy as np
from scipy.signal import convolve2d
from scipy.ndimage import label
from scipy.stats import moment
from mantid.geometry import RectangularDetector, GridDetector
import re
from enum import Enum

"""
This module contains the common utility classes used for peak integration algorithms
"""


class PEAK_MASK_STATUS(Enum):
    VALID = "Peak mask is valid"
    NPIX_MIN = "Peak mask has too few pixels"
    NCOL_MAX = "Peak mask has too many columns"
    NROW_MAX = "Peak mask has too many rows"
    DENSITY_MIN = "Peak mask density is below limit"
    VACANCY_MAX = "Peak mask has too many vacancies"
    ON_EDGE = "Peak mask is on the detector edge"
    NBINS_MIN = "Peak does not have enough non-zero bins"
    NO_PEAK = "No peak detected."


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
    def _split_string_trailing_int(name):
        *_, match = re.finditer(r"\d+", name)  # last match
        return name[: match.start()], name[match.start() :]  # prefix, num_str

    @staticmethod
    def _find_nearest_child_to_component(parent, comp, excl=None, ndepth=1):
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
            nearest_child = InstrumentArrayConverter._find_nearest_child_to_component(nearest_child, comp, ndepth=ndepth - 1)
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
            next_det = self._find_nearest_child_to_component(self.inst, det_ref, excl=bank_name, ndepth=2)
            if next_det is not None and next_det.getDistance(det_ref) < 1.1 * det_sep:
                # is considered adjacent
                next_col_prefix, next_col_str = self._split_string_trailing_int(next_det.getFullName())
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
        row_prefix, row_str = self._split_string_trailing_int(row_name)  # e.g. 'pixel', '0066'
        col_name = ci.name(ci.parent(det_idx))
        col_prefix, col_str = self._split_string_trailing_int(col_name)
        bank_name = bank.getFullName()
        bank_prefix, bank_str = self._split_string_trailing_int(bank_name)  # 'e.g. WISH/panel09/WISHPanel', '09'
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
        peak_data = PeakData(irow_peak, icol_peak, det_edges, detids, peak, self.ws)
        return peak_data


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
        # null init attributes
        self.initial_frac_width = None
        self.ixpk = None
        self.ixlo, self.ixhi = None, None
        self.ixlo_opt, self.ixhi_opt = None, None
        self.xpk, self.ypk, self.epk_sq = None, None, None
        self.peak_mask, self.non_bg_mask = None, None
        self.status = None
        self.intens, self.sig = 0, 0
        self.x_integrated_data = None

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

    def get_roi_on_detector(self, detids):
        isort = np.argsort(detids.flatten())
        ws_roi = exec_simpleapi_alg(
            "ExtractSpectra", InputWorkspace=self.ws, DetectorList=detids.flat[isort], OutputWorkspace=f"__roi{detids.flat[0]}"
        )
        return ws_roi, isort

    def integrate_xrange(self, xmin, xmax):
        # Pass in sorted detid list as ExtractSpectra does partial sorting
        ws_roi, isort = self.get_roi_on_detector(self.detids)
        ws_roi = exec_simpleapi_alg(
            "Integration", InputWorkspace=ws_roi, RangeLower=xmin, RangeUpper=xmax, IncludePartialBins=True, OutputWorkspace=ws_roi
        )
        # GroupDetectors sorts the workspace by detector ID so need to get correct order
        x_integrated_data = np.zeros(self.detids.size)
        x_integrated_data[isort] = np.squeeze(AnalysisDataService.retrieve(ws_roi).extractY())
        x_integrated_data = x_integrated_data.reshape(self.detids.shape)
        exec_simpleapi_alg("DeleteWorkspace", Workspace=ws_roi)
        return x_integrated_data

    def integrate_peak(
        self,
        frac_width,
        use_nearest,
        integrate_on_edge,
        optimise_mask,
        npk_min,
        density_min,
        nrow_max,
        ncol_max,
        min_npixels_per_vacancy,
        max_nvacancies,
        min_nbins,
        optimise_xwindow,
        threshold_i_over_sig,
    ):
        # get xlimits
        dx = frac_width * self.xpos
        self.xmin, self.xmax = self.xpos - 0.5 * dx, self.xpos + 0.5 * dx
        # find peak mask with intial x-limits
        self.x_integrated_data, self.peak_mask, self.non_bg_mask, peak_label = self.find_peak_mask(self.xmin, self.xmax, use_nearest)
        self.status = self._is_peak_mask_valid(
            self.peak_mask, peak_label, npk_min, density_min, nrow_max, ncol_max, min_npixels_per_vacancy, max_nvacancies, integrate_on_edge
        )
        # focus data - need to do this for plotting anyway even if mask not valid
        self.focus_data_in_detector_mask()
        self.ixmin, self.ixmax = self._find_xslice_indices(self.xmin, self.xmax)
        self.ixmin_opt, self.ixmax_opt = self.ixmin, self.ixmax
        self.xmin_opt, self.xmax_opt = self.xmin, self.xmax
        if self.status == PEAK_MASK_STATUS.VALID:
            # find bg and pk bins in focused spectrum by maximising I/sig
            self.find_peak_limits(self.ixmin, self.ixmax, optimise_xwindow, threshold_i_over_sig)
            if optimise_mask:
                # update the peak mask
                opt_xintegrated_data, opt_peak_mask, opt_non_bg_mask, peak_label = self.find_peak_mask(
                    self.xmin_opt, self.xmax_opt, use_nearest
                )
                # combine masks as optimal TOF window can truncate peak slightly
                opt_peak_mask = np.logical_or(self.peak_mask, opt_peak_mask)
                new_status = self._is_peak_mask_valid(
                    opt_peak_mask,
                    peak_label,
                    npk_min,
                    density_min,
                    nrow_max,
                    ncol_max,
                    min_npixels_per_vacancy,
                    max_nvacancies,
                    integrate_on_edge,
                )
                if new_status == PEAK_MASK_STATUS.VALID:
                    self.status = new_status
                    # refocus data and re-optimise TOF limits
                    self.x_integrated_data = opt_xintegrated_data
                    self.peak_mask = opt_peak_mask
                    self.non_bg_mask = np.logical_and(self.non_bg_mask, opt_non_bg_mask)
                    self.focus_data_in_detector_mask()
                    self.find_peak_limits(self.ixmin_opt, self.ixmax_opt, optimise_xwindow, threshold_i_over_sig)
            if np.sum(self.ypk[self.ixmin_opt : self.ixmax_opt] / np.sqrt(self.epk_sq[self.ixmin_opt : self.ixmax_opt]) > 1) < min_nbins:
                self.status = PEAK_MASK_STATUS.NBINS_MIN
            else:
                # do integration
                self.scale_intensity_by_bin_width()
                self.intens = np.sum(self.ypk[self.ixmin_opt : self.ixmax_opt])
                self.sig = np.sqrt(np.sum(self.epk_sq[self.ixmin_opt : self.ixmax_opt]))

    def find_peak_mask(self, xmin, xmax, use_nearest):
        x_integrated_data = self.integrate_xrange(xmin, xmax)
        _, ipeak2D = self.find_bg_pts_seed_skew(x_integrated_data.flatten())
        non_bg_mask = np.zeros(self.detids.shape, dtype=bool)
        non_bg_mask[np.unravel_index(ipeak2D, self.detids.shape)] = True
        labeled_array, num_features = label(non_bg_mask)
        # find label corresponding to peak
        peak_label = labeled_array[self.irow, self.icol]
        if peak_label == 0 and num_features > 0 and use_nearest:
            irows, icols = self._get_nearest_non_masked_index(non_bg_mask, self.irow, self.icol)
            # look for label corresponding to max. num peak pixels
            labels = np.unique(labeled_array[irows, icols])
            ilabel = np.argmax([np.sum(labeled_array == lab) for lab in labels])
            peak_label = labels[ilabel]
        peak_mask = labeled_array == peak_label
        return x_integrated_data, peak_mask, non_bg_mask, peak_label

    def _is_peak_mask_valid(
        self, peak_mask, peak_label, npk_min, density_min, nrow_max, ncol_max, min_npixels_per_vacancy, max_nvacancies, integrate_on_edge
    ):
        if not integrate_on_edge and np.any(np.logical_and(peak_mask, self.det_edges)):
            return PEAK_MASK_STATUS.ON_EDGE
        if peak_label == 0:
            return PEAK_MASK_STATUS.NO_PEAK
        if peak_mask.sum() < npk_min:
            return PEAK_MASK_STATUS.NPIX_MIN
        ncol = np.sum(self.peak_mask.sum(axis=0) > 0)
        if ncol > ncol_max:
            return PEAK_MASK_STATUS.NCOL_MAX
        nrow = np.sum(peak_mask.sum(axis=1) > 0)
        if nrow > nrow_max:
            return PEAK_MASK_STATUS.NROW_MAX
        density = peak_mask.sum() / (ncol * nrow)
        if density < density_min:
            return PEAK_MASK_STATUS.DENSITY_MIN
        if self._does_peak_have_vacancies(peak_mask, min_npixels_per_vacancy, max_nvacancies):
            return PEAK_MASK_STATUS.VACANCY_MAX
        return PEAK_MASK_STATUS.VALID

    @staticmethod
    def _does_peak_have_vacancies(peak_mask, min_npixels_per_vacancy, max_nvacancies):
        labeled_array, num_features = label(~peak_mask)
        nvac = 0
        for ifeature in range(1, num_features + 1):
            feature_mask = labeled_array == ifeature
            if not PeakData._does_mask_touch_window_edge(feature_mask):
                # is a vacancy within the peak region
                if feature_mask.sum() >= min_npixels_per_vacancy:
                    nvac += 1
                    if nvac > max_nvacancies:
                        return True
        return False

    @staticmethod
    def _does_mask_touch_window_edge(mask):
        return any(np.sum(mask, axis=0).astype(bool)[[0, -1]]) or any(np.sum(mask, axis=1).astype(bool)[[0, -1]])

    @staticmethod
    def _get_nearest_non_masked_index(mask, irow, icol):
        r, c = np.nonzero(mask)
        dist = (r - irow) ** 2 + (c - icol) ** 2
        imins = np.where(dist == dist.min())
        return np.array(r[imins]), np.array(c[imins])

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

    @staticmethod
    def _calc_snr(signal, error_sq):
        return np.sum(signal) / np.sqrt(np.sum(error_sq))

    def _focus_detids(self, detids):
        if not self.ws.isCommonBins():
            # get ws with just the selected detids
            ws_roi, _ = self.get_roi_on_detector(detids)
            # force common binning (same bin edges as first spectrum)
            ws_roi_spec = exec_simpleapi_alg(
                "ExtractSingleSpectrum", InputWorkspace=ws_roi, WorkspaceIndex=0, OutputWorkspace=ws_roi + "_spec"
            )
            exec_simpleapi_alg("RebinToWorkspace", WorkspaceToRebin=ws_roi, WorkspaceToMatch=ws_roi_spec, OutputWorkspace=ws_roi)
            ws_foc = exec_simpleapi_alg(
                "GroupDetectors",
                InputWorkspace=ws_roi,
                WorkspaceIndexList=range(0, len(detids)),
                OutputWorkspace=f"__foc{detids[0]}",
            )
            exec_simpleapi_alg("DeleteWorkspaces", WorkspaceList=[ws_roi, ws_roi_spec])
        else:
            ws_foc = exec_simpleapi_alg(
                "GroupDetectors",
                InputWorkspace=self.ws,
                DetectorList=",".join([str(id) for id in detids]),
                OutputWorkspace=f"__foc{detids[0]}",
            )
        return ws_foc

    def focus_data_in_detector_mask(self):
        # get background shell of non peak/feature pixels
        kernel = np.ones((3, 3))
        bg_shell_mask = convolve2d(self.peak_mask, kernel, mode="same")
        norm = convolve2d(np.ones(bg_shell_mask.shape), kernel, mode="same")
        bg_shell_mask = (bg_shell_mask / norm) > 0
        bg_shell_mask = np.logical_and(bg_shell_mask, ~self.non_bg_mask)
        # focus and subtract background shell
        ws_bg_foc = self._focus_detids(self.detids[bg_shell_mask])
        scale = exec_simpleapi_alg(
            "CreateSingleValuedWorkspace", DataValue=self.peak_mask.sum() / bg_shell_mask.sum(), OutputWorkspace="__scale"
        )
        exec_simpleapi_alg("Multiply", LHSWorkspace=ws_bg_foc, RHSWorkspace=scale, OutputWorkspace=ws_bg_foc)
        ws_pk_foc = self._focus_detids(self.detids[self.peak_mask])
        exec_simpleapi_alg("RebinToWorkspace", WorkspaceToRebin=ws_bg_foc, WorkspaceToMatch=ws_pk_foc, OutputWorkspace=ws_bg_foc)
        exec_simpleapi_alg("Subtract", LHSWorkspace=ws_pk_foc, RHSWorkspace=ws_bg_foc, OutputWorkspace=ws_pk_foc)
        ws_pk_foc = AnalysisDataService.retrieve(ws_pk_foc)
        self.ypk = ws_pk_foc.readY(0).copy()
        self.epk_sq = ws_pk_foc.readE(0).copy() ** 2
        self.xpk = ws_pk_foc.readX(0).copy()
        if len(self.xpk) > len(self.ypk):
            self.xpk = 0.5 * (self.xpk[:-1] + self.xpk[1:])  # convert to bin centers
        exec_simpleapi_alg("DeleteWorkspaces", WorkspaceList=[ws_bg_foc, ws_pk_foc, scale])

    def scale_intensity_by_bin_width(self):
        # multiply by bin width (as eventually want integrated intensity)
        dx = np.diff(self.xpk)
        self.ypk[1:] = self.ypk[1:] * dx
        self.ypk[0] = self.ypk[0] * dx[0]  # assume first has same dx as adjacent bin
        self.epk_sq[1:] = self.epk_sq[1:] * (dx**2)
        self.epk_sq[0] = self.epk_sq[0] * (dx[0] ** 2)

    def _find_xslice_indices(self, xmin, xmax):
        return np.argmin(abs(self.xpk - xmin)), np.argmin(abs(self.xpk - xmax)) + 1

    def find_peak_limits(self, ixmin, ixmax, optimise_xwindow, threshold_i_over_sig):
        # translate window to maximise I/sigma
        ixmin, ixmax, intens_over_sig = self.translate_xwindow_to_max_intens_over_sigma(self.ypk, self.epk_sq, ixmin, ixmax)
        if optimise_xwindow and intens_over_sig > threshold_i_over_sig:
            # find initial background points in tof/d-spacing window using skew method
            ibg, _ = self.find_bg_pts_seed_skew(self.ypk[ixmin:ixmax])
            # create mask and find largest contiguous region of peak bins
            skew_mask = np.ones(ixmax - ixmin, dtype=bool)
            skew_mask[ibg] = False
            labels, nlabel = label(skew_mask)
            if nlabel == 0:
                ilabel = 0
            else:
                ilabel = np.argmax([np.sum(labels == ilabel) for ilabel in range(1, nlabel + 1)]) + 1
            istart, iend = np.flatnonzero(labels == ilabel)[[0, -1]] + ixmin
            # expand window to maximise I/sig (good for when peak not entirely in window)
            self.ixmin_opt, self.ixmax_opt = self.optimise_xwindow_size_to_max_intens_over_sig(self.ypk, self.epk_sq, istart, iend + 1)
        else:
            self.ixmin_opt, self.ixmax_opt = ixmin, ixmax
        self.xmin_opt, self.xmax_opt = self.xpk[self.ixmin_opt], self.xpk[self.ixmax_opt - 1]

    @staticmethod
    def translate_xwindow_to_max_intens_over_sigma(signal, error_sq, ilo, ihi):
        dx = ihi - ilo
        kernel = np.ones(dx)
        istart = max(0, ilo - dx)
        iend = min(ihi + dx, signal.size + 1)
        # calc I/sigma using convolution - this assumes data are background subtracted
        i_over_sig = np.convolve(signal[istart:iend], kernel, mode="valid") / np.sqrt(
            np.convolve(error_sq[istart:iend], kernel, mode="valid")
        )
        imax_i_over_sigma = np.nanargmax(i_over_sig)
        ilo_opt = imax_i_over_sigma + istart
        ihi_opt = ilo_opt + dx
        return ilo_opt, ihi_opt, i_over_sig[imax_i_over_sigma]

    @staticmethod
    def optimise_xwindow_size_to_max_intens_over_sig(signal, error_sq, ilo, ihi, ntol=8, nbg=5):
        if ihi == ilo:
            ihi += 1
        # increase window to RHS
        nbg_rhs = nbg if signal.size - ihi > nbg + 1 else 0  # don't use bg if not enough points on rhs for one loop below
        nbad = 0
        bg = max(signal[ihi : ihi + nbg_rhs].mean(), 0) if nbg_rhs > 0 else 0  # default to 0 if not enough elements to rhs
        prev_IoverSig = PeakData._calc_snr(signal[ilo:ihi] - bg, error_sq[ilo:ihi])
        istep_hi = 0
        for istep_hi in range(1, signal.size - ihi - nbg_rhs):
            # set bg to be minimum bg observed in nbg window (good for double peaks) but force min value of 0 (post bg sub)
            bg = max(min(signal[ihi + istep_hi : ihi + istep_hi + nbg_rhs].mean(), bg), 0) if nbg_rhs > 0 else 0
            this_IoverSig = PeakData._calc_snr(signal[ilo : ihi + istep_hi] - bg, error_sq[ilo : ihi + istep_hi])
            if this_IoverSig > prev_IoverSig:
                prev_IoverSig = this_IoverSig
                nbad = 0
            else:
                nbad += 1
                if nbad > ntol:
                    break
        istep_hi -= nbad
        # increase window to LHS
        nbg_lhs = nbg if ilo > nbg else 0
        nbad = 0
        bg = signal[ilo - nbg_lhs : ilo].mean() if nbg_lhs > 0 else 0
        prev_IoverSig = PeakData._calc_snr(signal[ilo:ihi] - bg, error_sq[ilo:ihi])
        istep_lo = 0
        for istep_lo in range(1, ilo - nbg_lhs):
            bg = max(min(signal[ilo - istep_lo - nbg_lhs : ilo - istep_lo].mean(), bg), 0) if nbg_lhs > 0 else 0
            this_IoverSig = PeakData._calc_snr(signal[ilo - istep_lo : ihi] - bg, error_sq[ilo - istep_lo : ihi])
            if this_IoverSig > prev_IoverSig:
                prev_IoverSig = this_IoverSig
                nbad = 0
            else:
                nbad += 1
                if nbad > ntol:
                    break
        istep_lo -= nbad
        return ilo - istep_lo, ihi + istep_hi

    def update_peak_position(self):
        idet = np.argmax(self.x_integrated_data[self.peak_mask])
        det = self.detids[self.peak_mask][idet]
        self.irow, self.icol = np.where(self.detids == det)
        # update tof/d-spacing
        imax = np.argmax(self.ypk[self.ixmin_opt : self.ixmax_opt]) + self.ixmin_opt
        self.xpos = self.xpk[imax]
        return det, self.xpos

    def get_dTOF_over_TOF(self):
        # note dDSpacing/DSpacing = dTOF/TOF as TOF/dSpacing = const (for difa=tzero=0)
        return (self.xmax_opt - self.xmin_opt) / self.xpos

    def plot_integrated_peak(self, fig, ax, ipk, norm_func):
        # get color axis limits (for LogNorm scale)
        if np.any(self.x_integrated_data > 0):
            inonzero = self.x_integrated_data > 0
            vmin = self.x_integrated_data[inonzero].min()
            vmax = self.x_integrated_data[self.peak_mask].mean()
            if vmax <= vmin:
                vmax = self.x_integrated_data.max()
        else:
            vmin, vmax = 1, 1
        norm = norm_func(vmin=vmin, vmax=vmax)
        # limits for 1D plot
        ipad = (self.ixmax - self.ixmin) // 4  # extra portion of data shown outside the 1D window
        istart = max(min(self.ixmin, self.ixmin_opt) - ipad, 0)
        iend = min(max(self.ixmax, self.ixmax_opt) + ipad, len(self.xpk) - 1)
        # 2D plot - data integrated over optimal TOF range (not range for which mask determined)
        if not ax[0].images:
            # 2D colorfill
            img = ax[0].imshow(self.x_integrated_data, norm=norm)
            ax[0].plot(*np.where(self.peak_mask.T), "xw", label="mask")
            ax[0].plot(self.icol, self.irow, "or", label="cen")
            ax[0].set_xlabel("dColumn")
            ax[0].set_ylabel("dRow")
            cbar = fig.colorbar(img, orientation="horizontal", ax=ax[0], label="Intensity")
            cbar.ax.tick_params(labelsize=7, which="both")
            # 1D focused spectrum
            ax[1].axvline(self.xmin_opt, ls="--", color="b", label="Optimal window")
            ax[1].axvline(self.xmax_opt, ls="--", color="b")
            ax[1].errorbar(
                self.xpk[istart:iend],
                self.ypk[istart:iend],
                yerr=np.sqrt(self.epk_sq[istart:iend]),
                marker="o",
                markersize=3,
                ls="",
                color="k",
                label="data",
            )
            ax[1].axvline(self.xpos, ls="--", color=3 * [0.5], alpha=0.5, label="Centre")
            ax[1].axvline(self.xmin, ls=":", color="r", label="Initial window")
            ax[1].axvline(self.xmax, ls=":", color="r")
            ax[1].axhline(0, ls=":", color=3 * [0.5], alpha=0.5)
            ax[1].legend(fontsize=7, ncol=2, bbox_to_anchor=(0.75, 0), loc="lower center", bbox_transform=fig.transFigure)
            box = ax[1].get_position()
            ax[1].set_position([box.x0, box.y0 + 0.1, box.width, box.height - 0.1])
            ax[1].set_xlabel(r"TOF ($\mu$s)")
            ax[1].set_ylabel("Intensity")
        else:
            # update 2D colorfill
            img = ax[0].images[0]
            img.set_norm(norm)
            img.set_data(self.x_integrated_data)
            xmask, ymask = np.where(self.peak_mask.T)
            ax[0].lines[0].set_xdata(xmask)
            ax[0].lines[0].set_ydata(ymask)
            ax[0].lines[1].set_xdata([self.icol])
            ax[0].lines[1].set_ydata([self.irow])
            img.set_extent([-0.5, self.x_integrated_data.shape[1] - 0.5, self.x_integrated_data.shape[0] - 0.5, -0.5])
            # update 1D focused spectrum
            # vlines
            xmin_line, xmax_line, data, xpos_line, xmin_init_line, xmax_init_line, y0 = ax[1].lines
            xmin_line.set_xdata([self.xmin_opt])
            xmax_line.set_xdata([self.xmax_opt])
            xmin_init_line.set_xdata([self.xmin])
            xmax_init_line.set_xdata([self.xmax])
            xpos_line.set_xdata([self.xpos])
            # spectrum
            yerr = np.sqrt(self.epk_sq[istart:iend])
            data.set_xdata(self.xpk[istart:iend])
            data.set_ydata(self.ypk[istart:iend])
            ax[1].collections[0].set_segments(
                [
                    np.array([[x, ybot], [x, ytop]])
                    for x, ybot, ytop in zip(data.get_xdata(), data.get_ydata() - yerr, data.get_ydata() + yerr)
                ]
            )
        # format 2D colorfill
        title_str = (
            f"{ipk} ({','.join(str(self.hkl)[1:-1].split())}) "
            rf"$\lambda$={np.round(self.wl, 2)} $\AA$ "
            rf"$2\theta={2 * np.degrees(self.theta):.0f}^\circ$"
            f"\n{self.status.value}"
        )
        ax[0].set_title(title_str)
        # format 1D
        intens_over_sig = round(self.intens / self.sig, 2) if self.sig > 0 else 0
        ax[1].set_title(f"I/sig = {intens_over_sig}")
        ax[1].relim()
        ax[1].autoscale_view()


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
