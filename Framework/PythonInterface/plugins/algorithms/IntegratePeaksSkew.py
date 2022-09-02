# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2022 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
from mantid.api import (DataProcessorAlgorithm, AlgorithmFactory, Progress, MatrixWorkspaceProperty,
                        IPeaksWorkspaceProperty, FileProperty, FileAction, WorkspaceUnitValidator)
from mantid.kernel import (Direction, FloatBoundedValidator, IntBoundedValidator, EnabledWhenProperty,
                           PropertyCriterion, logger)
import numpy as np
from scipy.signal import convolve2d
from scipy.ndimage import label
from scipy.stats import moment
from mantid.geometry import RectangularDetector, GridDetector
import re
from enum import Enum


class PEAK_MASK_STATUS(Enum):
    VALID = "Peak mask is valid"
    NPIX_MIN = "Peak mask has too few pixels"
    NCOL_MAX = "Peak mask has too many columns"
    NROW_MAX = "Peak mask has too many rows"
    DENSITY_MIN = "Peak mask density is below limit"
    VACANCY_MAX = "Peak mask has too many vacancies"
    ON_EDGE = "Peak mask is on the detector edge"
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
        if any(self.inst[icomp] for icomp in range(self.inst.nelements()) if
               isinstance(self.inst[icomp], RectangularDetector) or isinstance(self.inst[icomp], GridDetector)):
            # might not be true for all components due to presence of monitors etc.
            self.get_detid_array = self._get_detid_array_rect_detector
        else:
            self.get_detid_array = self._get_detid_array_comp_assembly

    @staticmethod
    def _split_string_trailing_int(name):
        *_, match = re.finditer(r'\d+', name)  # last match
        return name[:match.start()], name[match.start():]  # prefix, num_str

    @staticmethod
    def _find_nearest_child_to_component(parent, comp, excl=None, ndepth=1):
        dist = np.inf
        nearest_child = None
        for ichild in range(parent.nelements()):
            child = parent[ichild]
            if hasattr(child, 'nelements'):
                while child.nelements() == 1:
                    child = child[0]
            is_correct_type = 'CompAssembly' in child.type() or child.type() == 'DetectorComponent'
            is_not_excluded = excl is None or child.getFullName() != excl
            if is_correct_type and is_not_excluded:
                this_dist = child.getDistance(comp)
                if this_dist < dist:
                    dist = this_dist
                    nearest_child = child
        if ndepth > 1:
            nearest_child = InstrumentArrayConverter._find_nearest_child_to_component(nearest_child, comp,
                                                                                      ndepth=ndepth - 1)
        return nearest_child

    def _find_nearest_adjacent_col_component(self, dcol, col, ncols, bank_name, col_prefix, col_str, delim, ncols_edge):
        next_col_prefix, next_col_str = None, None
        isLHS = np.any(dcol + col <= ncols_edge)
        isRHS = np.any(dcol + col >= ncols - ncols_edge + 1)
        if isLHS or isRHS:
            # get tube/col at each end of detector to get tube separation
            detLHS = self.inst.getComponentByName(
                delim.join([bank_name, f'{col_prefix}{1:0{len(col_str)}d}']))
            detRHS = self.inst.getComponentByName(
                delim.join([bank_name, f'{col_prefix}{ncols:0{len(col_str)}d}']))
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
        next_col_prefix, next_col_str, isLHS, isRHS = self._find_nearest_adjacent_col_component(dcol, col, ncols,
                                                                                                bank_name,
                                                                                                col_prefix, col_str,
                                                                                                delim, ncols_edge)

        # loop over row/cols and get detids
        detids = np.zeros(dcol.shape, int)
        for icol in range(dcol.shape[1]):
            new_col = col + dcol[0, icol]
            col_comp_name = None  # None indicates no column component (tube) found in this or adjacent bank
            if 0 < new_col <= ncols:  # indexing starts from 1
                # column comp in this bank
                col_comp_name = delim.join([bank_name, f'{col_prefix}{new_col:0{len(col_str)}d}'])
            elif next_col_str is not None:
                # adjacent column component found in another bank
                new_col = int(next_col_str) + new_col
                if isRHS:
                    new_col -= (ncols + 1)
                col_comp_name = f'{next_col_prefix}{new_col:0{len(next_col_str)}d}'
            for irow in range(dcol.shape[0]):
                if col_comp_name is not None:
                    new_row = row + drow[irow, 0]
                    row_comp_name = f'{row_prefix}{new_row:0{len(row_str)}d}'
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
        row_edges = np.logical_or(drow <= ndet_per_spec * nrows_edge - row,
                                  drow > nrows - row - ndet_per_spec * nrows_edge)
        detector_edges = np.logical_or(row_edges, col_edges)
        # row_peak, icol_peak
        irow_peak = np.where(drow[:, 0] == 0)[0][0]
        icol_peak = np.where(dcol[0, :] == 0)[0][0]
        return detids, detector_edges, irow_peak, icol_peak

    def _get_detid_array_rect_detector(self, bank, detid, row, col, drows, dcols, nrows_edge, ncols_edge):
        col_step, row_step = bank.idstep(), bank.idstepbyrow()  # step in detID along col and row
        if bank.idfillbyfirst_y():
            col_step, row_step = row_step, col_step
        # need to adjust range depending on whether above min/max row/col
        drow_vec = np.arange(max(0, row - drows), min(row + drows + 1, bank.xpixels())) - row
        dcol_vec = np.arange(max(0, col - dcols), min(col + dcols + 1, bank.ypixels())) - col
        dcol, drow = np.meshgrid(dcol_vec, drow_vec)
        detids = detid + dcol * col_step + drow * row_step
        # create bool mask for detector edges
        detector_edges = np.logical_or.reduce((drow <= -row + nrows_edge - 1, dcol <= -col + ncols_edge - 1,
                                               drow >= bank.xpixels() - nrows_edge - row,
                                               dcol >= bank.ypixels() - ncols_edge - col))
        # get indices of peak centre
        irow_peak = np.where(drow_vec == 0)[0][0]
        icol_peak = np.where(dcol_vec == 0)[0][0]
        return detids, detector_edges, irow_peak, icol_peak

    def get_peak_region_array(self, peak, detid, bank_name, drows, dcols, nrows_edge, ncols_edge):
        """
        :param peak: peak object
        :param detid: detector id of peak (from peak table)
        :param bank_name: bank name on which detector resides (from peak table)
        :param dpixel: width of detector window in pixels (along row and columns)
        :return signal: 3D numpy array containing signal/intensities (row x cols x bins)
        :return errors: 3D numpy array containing errors (row x cols x bins)
        :return irow_peak: index of peak position along first dim (row)
        :return icol_peak: index of peak position along second dim (col)
        :return ispec: spectrum index corresponding to detid of peak
        :return edges: bool mask True for pixels on the edge of a detector bank/panel
        """
        bank = self.inst.getComponentByName(bank_name)
        row, col = peak.getRow(), peak.getCol()
        detids, detector_edges, irow_peak, icol_peak = self.get_detid_array(bank, detid, row, col, drows, dcols,
                                                                            nrows_edge, ncols_edge)
        # get signal and error from each spectrum
        ispecs = np.array(self.ws.getIndicesFromDetectorIDs(
            [int(d) for d in detids.flatten()])).reshape(detids.shape)
        signal = np.zeros((*ispecs.shape, self.ws.blocksize()))
        errors = np.zeros(signal.shape)
        for irow in range(signal.shape[0]):
            for icol in range(signal.shape[1]):
                ispec = int(ispecs[irow, icol])
                signal[irow, icol, :] = self.ws.readY(ispec)
                errors[irow, icol, :] = self.ws.readE(ispec)
        # get x bin centers
        xvals = self.ws.readX(int(ispecs[irow_peak, icol_peak]))
        if len(xvals) > self.ws.blocksize():
            xvals = 0.5 * (xvals[:-1] + xvals[1:])  # convert to bin centers
        return xvals, signal, errors, irow_peak, icol_peak, detector_edges, detids


def is_peak_mask_valid(peak_mask, peak_label, npk_min, density_min, nrow_max, ncol_max,
                       min_npixels_per_vacancy, max_nvacancies):
    if peak_label == 0:
        return False, PEAK_MASK_STATUS.NO_PEAK
    if peak_mask.sum() < npk_min:
        return False, PEAK_MASK_STATUS.NPIX_MIN
    ncol = np.sum(peak_mask.sum(axis=0) > 0)
    if ncol > ncol_max:
        return False, PEAK_MASK_STATUS.NCOL_MAX
    nrow = np.sum(peak_mask.sum(axis=1) > 0)
    if nrow > nrow_max:
        return False, PEAK_MASK_STATUS.NROW_MAX
    density = peak_mask.sum() / (ncol * nrow)
    if density < density_min:
        return False, PEAK_MASK_STATUS.DENSITY_MIN
    if does_peak_have_vacancies(peak_mask, min_npixels_per_vacancy, max_nvacancies):
        return False, PEAK_MASK_STATUS.VACANCY_MAX
    return True, PEAK_MASK_STATUS.VALID


def does_peak_have_vacancies(peak_mask, min_npixels_per_vacancy=2, max_nvacancies=1):
    labeled_array, num_features = label(~peak_mask)
    nvac = 0
    for ifeature in range(1, num_features + 1):
        feature_mask = labeled_array == ifeature
        if not does_mask_touch_window_edge(feature_mask):
            # is a vacancy within the peak region
            if feature_mask.sum() >= min_npixels_per_vacancy:
                nvac += 1
                if nvac > max_nvacancies:
                    return True
    return False


def does_mask_touch_window_edge(peak_mask):
    return any(np.sum(peak_mask, axis=0).astype(bool)[[0, -1]]) or any(np.sum(peak_mask, axis=1).astype(bool)[[0, -1]])


def get_nearest_non_masked_index(mask, irow, icol):
    r, c = np.nonzero(mask)
    dist = ((r - irow) ** 2 + (c - icol) ** 2)
    imins = np.where(dist == dist.min())
    return np.array(r[imins]), np.array(c[imins])


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
        if this_skew >= prev_skew:
            istart -= 1
            break
        else:
            prev_skew = this_skew
    return ibg_seed[isort[istart:iend]], ibg_seed[isort[:istart]]  # bg, non-bg


def calc_snr(signal, error_sq):
    return np.sum(signal) / np.sqrt(np.sum(error_sq))


def optimise_tof_window_intens_over_sig(signal, error_sq, ilo, ihi, ntol=8, nbg=5):
    if ihi == ilo:
        ihi += 1
    # increase window to RHS
    nbg_rhs = nbg if signal.size - ihi > nbg + 1 else 0  # don't use bg if not enough points on rhs for one loop below
    nbad = 0
    bg = max(signal[ihi:ihi + nbg_rhs].mean(), 0) if nbg_rhs > 0 else 0  # default to 0 if not enough elements to rhs
    prev_IoverSig = calc_snr(signal[ilo:ihi] - bg, error_sq[ilo:ihi])
    istep_hi = 0
    for istep_hi in range(1, signal.size - ihi - nbg_rhs):
        # set bg to be minimum bg observed in nbg window (good for double peaks) but force min value of 0 (post bg sub)
        bg = max(min(signal[ihi + istep_hi:ihi + istep_hi + nbg_rhs].mean(), bg), 0) if nbg_rhs > 0 else 0
        this_IoverSig = calc_snr(signal[ilo:ihi + istep_hi] - bg, error_sq[ilo:ihi + istep_hi])
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
    bg = signal[ilo - nbg_lhs:ilo].mean() if nbg_lhs > 0 else 0
    prev_IoverSig = calc_snr(signal[ilo:ihi] - bg, error_sq[ilo:ihi])
    istep_lo = 0
    for istep_lo in range(1, ilo - nbg_lhs):
        bg = max(min(signal[ilo - istep_lo - nbg_lhs:ilo - istep_lo].mean(), bg), 0) if nbg_lhs > 0 else 0
        this_IoverSig = calc_snr(signal[ilo - istep_lo:ihi] - bg, error_sq[ilo - istep_lo:ihi])
        if this_IoverSig > prev_IoverSig:
            prev_IoverSig = this_IoverSig
            nbad = 0
        else:
            nbad += 1
            if nbad > ntol:
                break
    istep_lo -= nbad
    return ilo - istep_lo, ihi + istep_hi


def find_peak_limits(signal, error_sq, ilo, ihi):
    # find initial background points in tof window using skew method
    ibg, _ = find_bg_pts_seed_skew(signal[ilo:ihi])
    # create mask and find largest contiguous region of peak bins
    skew_mask = np.ones(ihi - ilo, dtype=bool)
    skew_mask[ibg] = False
    labels, nlabel = label(skew_mask)
    if nlabel == 0:
        ilabel = 0
    else:
        ilabel = np.argmax([np.sum(labels == ilabel) for ilabel in range(1, nlabel + 1)]) + 1
    istart, iend = np.flatnonzero(labels == ilabel)[[0, -1]] + ilo
    # expand window to maximise I/sig (good for when peak not entirely in window)
    return optimise_tof_window_intens_over_sig(signal, error_sq, istart, iend + 1)  #


def focus_data_in_detector_mask(signal, error, peak_mask, non_bg_mask, ixpk):
    # focus peak in TOF
    ypk = signal[peak_mask].sum(axis=0)
    epk_sq = np.sum(error[peak_mask] ** 2, axis=0)
    # get background shell of non peak/feature pixels
    kernel = np.ones((3, 3))
    bg_shell_mask = convolve2d(peak_mask, kernel, mode='same')
    norm = convolve2d(np.ones(bg_shell_mask.shape), kernel, mode='same')
    bg_shell_mask = (bg_shell_mask / norm) > 0
    bg_shell_mask = np.logical_and(bg_shell_mask, ~non_bg_mask)
    # focus background shell
    scale = peak_mask.sum() / bg_shell_mask.sum()
    ybg = scale * signal[bg_shell_mask].sum(axis=0)
    ebg_sq = (scale ** 2) * np.sum(error[bg_shell_mask] ** 2, axis=0)
    # replace zero errors in epk and ebg_sq  with avg. error in same quarter of spectrum in background data
    width = len(ypk) / 4
    iquarter = ixpk // width
    ebg_sq_subset = ebg_sq[int(iquarter * width):int((iquarter + 1) * width)]
    ebg_sq_avg = np.mean(ebg_sq_subset[ebg_sq_subset > 0])
    epk_sq[epk_sq == 0] = ebg_sq_avg
    ebg_sq[ebg_sq == 0] = ebg_sq_avg
    # subtract bg from focused peak
    ypk = ypk - ybg  # removes background and powder lines (roughly line up in tof)
    epk_sq = epk_sq + ebg_sq
    return ypk, epk_sq


def find_peak_mask(signal, ixlo, ixhi, irow, icol, use_nearest):
    _, ipeak2D = find_bg_pts_seed_skew(signal[:, :, ixlo:ixhi].sum(axis=2).flatten())
    non_bg_mask = np.zeros(signal.shape[0:2], dtype=bool)
    non_bg_mask[np.unravel_index(ipeak2D, signal.shape[0:2])] = True
    labeled_array, num_features = label(non_bg_mask)
    # find label corresponding to peak
    peak_label = labeled_array[irow, icol]
    if peak_label == 0 and num_features > 0 and use_nearest:
        irows, icols = get_nearest_non_masked_index(non_bg_mask, irow, icol)
        # look for label corresponding to max. num peak pixels
        labels = np.unique(labeled_array[irows, icols])
        ilabel = np.argmax([np.sum(labeled_array == lab) for lab in labels])
        peak_label = labels[ilabel]
        # throw warning if distance is greater than half window?
    peak_mask = labeled_array == peak_label
    return peak_mask, non_bg_mask, peak_label


def plot_integrated_peak(fig, ax, xpk, ypk, epk_sq, image_data, peak_mask, irow, icol, ixlo, ixhi, ixlo_opt, ixhi_opt,
                         pk, ipk, status, intens, sig, norm_func):
    # limits for 1D plot
    ipad = int((ixhi - ixlo) / 2)  # extra portion of data shown outside the 1D window
    istart = max(min(ixlo, ixlo_opt) - ipad, 0)
    iend = min(max(ixhi, ixhi_opt) + ipad, len(xpk) - 1)
    # 2D plot - data integrated over optimal TOF range (not range for which mask determined)
    img = ax[0].imshow(image_data, norm=norm_func(vmax=image_data[peak_mask].mean()))
    ax[0].plot(*np.where(peak_mask.T), 'xw')
    ax[0].plot(icol, irow, 'or')
    title_str = f"{ipk} ({str(pk.getIntHKL())[1:-1]}) " \
                rf"$\lambda$={np.round(pk.getWavelength(), 2)} $\AA$" \
                f"\n{status.value}"
    ax[0].set_title(title_str)
    ax[0].set_xlabel('dColumn')
    ax[0].set_ylabel('dRow')
    # 1D plot
    ax[1].axvline(xpk[ixlo_opt], ls='--', color='b', label='Optimal window')
    ax[1].axvline(xpk[ixhi_opt - 1], ls='--', color='b')
    ax[1].errorbar(xpk[istart:iend], ypk[istart:iend], yerr=np.sqrt(epk_sq[istart:iend]),
                   marker='o', markersize=3, capsize=2, ls='', color='k', label='data')
    ax[1].axvline(pk.getTOF(), ls='--', color='k', label='Centre')
    ax[1].axvline(xpk[ixlo], ls=':', color='r', label='Initial window')
    ax[1].axvline(xpk[ixhi - 1], ls=':', color='r')
    ax[1].axhline(0, ls=':', color='k')
    ax[1].legend(fontsize=7, loc=1, ncol=2)
    # figure formatting
    intens_over_sig = round(intens / sig, 2) if sig > 0 else 0
    ax[1].set_title(f'I/sig = {intens_over_sig}')
    ax[1].set_xlabel(r'TOF ($\mu$s)')
    ax[1].set_ylabel('Intensity')
    cbar = fig.colorbar(img, orientation='horizontal', ax=ax[0], label='Intensity')
    cbar.ax.tick_params(labelsize=7, which='both')
    ax[1].relim()
    ax[1].autoscale_view()
    return cbar  # need object returned so can cleanup after fig saved to pdf


class IntegratePeaksSkew(DataProcessorAlgorithm):

    def name(self):
        return "IntegratePeaksSkew"

    def category(self):
        return "Diffraction\\Reduction"

    def seeAlso(self):
        return ["IntegratePeaksMD", "IntegrateEllipsoids"]

    def summary(self):
        return "Integrates single-crystal peaks in a MatrixWorkspace by identifying the peak pixels in a window on " \
               "the detector by minimising the skew of the points in the background. The TOF extent of the peak is " \
               "determined by maximising I/:math:`\\sigma` for the peak pixels identified using the skew method. "

    def PyInit(self):
        # Input
        self.declareProperty(MatrixWorkspaceProperty(name="InputWorkspace", defaultValue="", direction=Direction.Input,
                                                     validator=WorkspaceUnitValidator("TOF")),
                             doc="A MatrixWorkspace to integrate (x-axis must be TOF).")
        self.declareProperty(IPeaksWorkspaceProperty(name="PeaksWorkspace",
                                                     defaultValue="",
                                                     direction=Direction.Input),
                             doc="A PeaksWorkspace containing the peaks to integrate.")

        #   window parameters
        self.declareProperty(name="DRows", defaultValue=8, direction=Direction.Input,
                             validator=IntBoundedValidator(lower=1),
                             doc="Determines the size of the window around a peak on the detector. "
                                 "The number of rows in the window are NRows = 2*DRows + 1.")
        self.declareProperty(name="DCols", defaultValue=8, direction=Direction.Input,
                             validator=IntBoundedValidator(lower=1),
                             doc="Determines the size of the window around a peak on the detector. "
                                 "The number of columns in the window are NCols = 2*DCols + 1.")
        self.declareProperty(name='FractionalTOFWindow', defaultValue=0.0, direction=Direction.Input,
                             validator=FloatBoundedValidator(lower=0.0, upper=1.0),
                             doc="dTOF/TOF window best chosen from forward-scattering bank with worst resolution.")
        condition_to_use_resn = EnabledWhenProperty('FractionalTOFWindow', PropertyCriterion.IsEqualTo, "0")
        self.declareProperty(name="BackscatteringTOFResolution", defaultValue=0.04, direction=Direction.Input,
                             validator=FloatBoundedValidator(lower=0, upper=1.0),
                             doc="dTOF/TOF of window for peaks at back-scattering (resolution dominated by moderator "
                                 "contribution, dT0/T0, and uncertainty in path length dL/L which is assumed constant "
                                 "for all pixels).")
        self.setPropertySettings("BackscatteringTOFResolution", condition_to_use_resn)
        self.declareProperty(name="ThetaWidth", defaultValue=0.015, direction=Direction.Input,
                             validator=FloatBoundedValidator(lower=0),
                             doc="dTheta resolution (estimated from width at forward scattering minus contribution "
                                 "from moderator, dT0/T0, and path length dL/L).")
        self.setPropertySettings("ThetaWidth", condition_to_use_resn)
        self.declareProperty(name="OptimiseMask", defaultValue=False, direction=Direction.Input,
                             doc="Redo peak mask using optimal TOF window discovered (the original mask is found from "
                                 "the integrated intensity over a TOF window determined from the resolution "
                                 "parameters). A new optimal TOF window is then found using the new peak mask."
                                 "Note this can be helpful if resolution parameters or peak centres are not very "
                                 "accurate.")
        self.setPropertyGroup("DRows", "Integration Window Parameters")
        self.setPropertyGroup("DCols", "Integration Window Parameters")
        self.setPropertyGroup('FractionalTOFWindow', "Integration Window Parameters")
        self.setPropertyGroup("BackscatteringTOFResolution", "Integration Window Parameters")
        self.setPropertyGroup("ThetaWidth", "Integration Window Parameters")
        self.setPropertyGroup("OptimiseMask", "Integration Window Parameters")
        # peak mask validators
        self.declareProperty(name="IntegrateIfOnEdge", defaultValue=False, direction=Direction.Input,
                             doc="Integrate peaks that contain pixels on edge of the detector.")
        self.declareProperty(name="NRowsEdge", defaultValue=1, direction=Direction.Input,
                             validator=IntBoundedValidator(lower=1),
                             doc="Masks including pixels on rows NRowsEdge from the detector edge are "
                                 "defined as on the edge.")
        self.declareProperty(name="NColsEdge", defaultValue=1, direction=Direction.Input,
                             validator=IntBoundedValidator(lower=1),
                             doc="Masks including pixels on cols NColsEdge from the detector edge are "
                                 "defined as on the edge.")
        self.declareProperty(name="NPixMin", defaultValue=3, direction=Direction.Input,
                             validator=IntBoundedValidator(lower=1),
                             doc="Minimum number of pixels contributing to a peak")
        self.declareProperty(name="DensityPixMin", defaultValue=0.35, direction=Direction.Input,
                             validator=FloatBoundedValidator(lower=0),
                             doc="Minimum density of peak pixels in bounding box")
        self.declareProperty(name="NRowMax", defaultValue=15, direction=Direction.Input,
                             validator=IntBoundedValidator(lower=1),
                             doc="Maximum number of rows in peak mask (note on WISH rows are equivalent to pixels).")
        self.declareProperty(name="NColMax", defaultValue=15, direction=Direction.Input,
                             validator=IntBoundedValidator(lower=1),
                             doc="Maximum number of columns in peak mask (note on WISH cols are equivalent to tubes).")
        self.declareProperty(name="NVacanciesMax", defaultValue=0, direction=Direction.Input,
                             validator=IntBoundedValidator(lower=0),
                             doc="Maximum number of vacancies (contiguous regions of non-peak pixels entirely "
                                 "contained within the peak mask) for a valid peak.")
        self.declareProperty(name="NPixPerVacancyMin", defaultValue=1, direction=Direction.Input,
                             validator=IntBoundedValidator(lower=1),
                             doc="Minimum number of pixels in a vacancy")
        self.setPropertyGroup("IntegrateIfOnEdge", "Peak Mask Validation")
        self.setPropertyGroup("NRowsEdge", "Peak Mask Validation")
        self.setPropertyGroup("NColsEdge", "Peak Mask Validation")
        self.setPropertyGroup("NPixMin", "Peak Mask Validation")
        self.setPropertyGroup("DensityPixMin", "Peak Mask Validation")
        self.setPropertyGroup("NRowMax", "Peak Mask Validation")
        self.setPropertyGroup("NColMax", "Peak Mask Validation")
        self.setPropertyGroup("NVacanciesMax", "Peak Mask Validation")
        self.setPropertyGroup("NPixPerVacancyMin", "Peak Mask Validation")
        # peak finding
        self.declareProperty(name="UseNearestPeak", defaultValue=False, direction=Direction.Input,
                             doc="Find nearest peak pixel if peak position is in a background pixel.")
        self.declareProperty(name="UpdatePeakPosition", defaultValue=False, direction=Direction.Input,
                             doc="If True then the peak position will be updated to be the detid with the "
                                 "largest integrated counts over the optimised TOF window, and the peak TOF will be "
                                 "taken as the maximum of the focused data in the TOF window.")
        self.setPropertyGroup("UseNearestPeak", "Peak Finding")
        self.setPropertyGroup("UpdatePeakPosition", "Peak Finding")
        # plotting
        self.declareProperty(FileProperty("OutputFile", "", FileAction.OptionalSave, ".pdf"),
                             "Optional file path in which to write diagnostic plots (note this will slow the "
                             "execution of algorithm).")
        self.setPropertyGroup("OutputFile", "Plotting")

        # Output
        self.declareProperty(IPeaksWorkspaceProperty(name="OutputWorkspace",
                                                     defaultValue="",
                                                     direction=Direction.Output),
                             doc="The output PeaksWorkspace will be a copy of the input PeaksWorkspace with the"
                                 " integrated intensities.")

    def validateInputs(self):
        issues = dict()
        # check peak size limits are consistent with window size
        drows = self.getProperty("DRows").value
        dcols = self.getProperty("DCols").value
        nrow_max = self.getProperty("NRowMax").value
        ncol_max = self.getProperty("NColMax").value
        if nrow_max > 2 * drows + 1:
            issues["NRowMax"] = "NRowMax exceeds window size."
        if ncol_max > 2 * dcols + 1:
            issues["NColMax"] = "NColMax exceeds window size."
        npk_min = self.getProperty("NPixMin").value
        if npk_min > (2 * drows + 1) * (2 * dcols + 1):
            issues["NPixMin"] = "NPixMin exceeds number of pixels in the window."
        # check valid peak workspace
        ws = self.getProperty("InputWorkspace").value
        pk_ws = self.getProperty("PeaksWorkspace").value
        if ws.getInstrument().getName() != pk_ws.getInstrument().getName():
            issues["PeaksWorkspace"] = "PeaksWorkspace must have same instrument as the InputWorkspace."
        if pk_ws.getNumberPeaks() < 1:
            issues["PeaksWorkspace"] = "PeaksWorkspace must have at least 1 peak."
        return issues

    def PyExec(self):
        # get input
        ws = self.getProperty("InputWorkspace").value
        pk_ws = self.getProperty("PeaksWorkspace").value
        # peak window parameters
        frac_tof_window = self.getProperty('FractionalTOFWindow').value
        dt0_over_t0 = self.getProperty("BackscatteringTOFResolution").value
        dth = self.getProperty("ThetaWidth").value
        drows = self.getProperty("DRows").value
        dcols = self.getProperty("DCols").value
        optimise_mask = self.getProperty("OptimiseMask").value
        # peak mask validation
        integrate_on_edge = self.getProperty("IntegrateIfOnEdge").value
        nrows_edge = self.getProperty("NRowsEdge").value
        ncols_edge = self.getProperty("NColsEdge").value
        npk_min = self.getProperty("NPixMin").value
        density_min = self.getProperty("DensityPixMin").value
        nrow_max = self.getProperty("NRowMax").value
        ncol_max = self.getProperty("NColMax").value
        max_nvacancies = self.getProperty("NVacanciesMax").value
        min_npixels_per_vacancy = self.getProperty("NPixPerVacancyMin").value
        # peak finding
        use_nearest = self.getProperty("UseNearestPeak").value
        update_peak_pos = self.getProperty("UpdatePeakPosition").value
        # plotting
        plot_filename = self.getProperty("OutputFile").value

        array_converter = InstrumentArrayConverter(ws)

        # setup progress bar
        prog_reporter = Progress(self, start=0.0, end=1.0, nreports=pk_ws.getNumberPeaks())

        # Empty table workspace (clone and delete so as to preserve UB, sample, history etc.)
        pk_ws_int = self.child_CloneWorkspace(InputWorkspace=pk_ws, OutputWorkspace="_temp")  # temp for ws in/out same
        self.child_DeleteTableRows(TableWorkspace=pk_ws_int, Rows=range(pk_ws_int.getNumberPeaks()))
        # get spectrum indices for all peaks in table
        detids = pk_ws.column('DetID')
        bank_names = pk_ws.column('BankName')
        tofs = pk_ws.column('TOF')
        if plot_filename:
            import matplotlib.pyplot as plt
            from matplotlib.backends.backend_pdf import PdfPages
            from matplotlib.colors import LogNorm
            fig, ax = plt.subplots(1, 2, subplot_kw={'projection': 'mantid'})
            fig.subplots_adjust(wspace=0.5)  # ensure plenty spaxce between subplots (want to avoid slow tight_layout)
            try:
                pdf = PdfPages(plot_filename)
            except OSError:
                raise RuntimeError(f"OutputFile ({plot_filename}) could not be opened - please check it is not open by "
                                   f"another programme and that the user has permission to write to that directory.")
        irows_delete = []
        for ipk, pk in enumerate(pk_ws):
            # copy pk to output peak workspace
            pk_ws_int.addPeak(pk)
            pk = pk_ws_int.getPeak(pk_ws_int.getNumberPeaks() - 1)  # don't overwrite pk in input ws
            # check that peak is in a valid detector
            detid = detids[ipk]
            detector_info = ws.detectorInfo()
            invalid_detector = False
            try:
                det_idx = detector_info.indexOf(detid)
                invalid_detector = detector_info.isMonitor(det_idx) or detector_info.isMasked(det_idx)
            except IndexError:
                invalid_detector = True  # no index when e.g. predicted peak outside detector (detid = -1)
            if invalid_detector:
                logger.error("Peak with index {ipk} is not in a valid detector (with ID {detid}).")
                continue  # skip peak - don't plot as no data to retrieve
            # get data array in window around peak region
            xpk, signal, error, irow, icol, det_edges, dets = array_converter.get_peak_region_array(pk, detid,
                                                                                                    bank_names[ipk],
                                                                                                    drows, dcols,
                                                                                                    nrows_edge,
                                                                                                    ncols_edge)
            # get TOF window using resolution parameters
            if frac_tof_window > 0:
                dTOF = tofs[ipk] * frac_tof_window
            else:
                dTOF = tofs[ipk] * np.sqrt(dt0_over_t0 ** 2 + (dth / np.tan(pk.getScattering() / 2)) ** 2)
            ixlo = np.argmin(abs(xpk - (tofs[ipk] - 0.5 * dTOF)))
            ixhi = np.argmin(abs(xpk - (tofs[ipk] + 0.5 * dTOF)))
            ixpk = np.argmin(abs(xpk - tofs[ipk]))
            # find 2D peak in detector window
            peak_mask, non_bg_mask, peak_label = find_peak_mask(signal, ixlo, ixhi, irow, icol, use_nearest)
            intens, sig = 0.0, 0.0
            integrated = False
            if not integrate_on_edge and np.any(np.logical_and(peak_mask, det_edges)):
                status = PEAK_MASK_STATUS.ON_EDGE
            else:
                is_valid_mask, status = is_peak_mask_valid(peak_mask, peak_label, npk_min, density_min,
                                                           nrow_max, ncol_max, min_npixels_per_vacancy, max_nvacancies)
                if is_valid_mask:
                    ypk, epk_sq = focus_data_in_detector_mask(signal, error, peak_mask, non_bg_mask, ixpk)
                    # find bg and pk bins in focused spectrum by maximising I/sig
                    ixlo_opt, ixhi_opt = find_peak_limits(ypk, epk_sq, ixlo, ixhi)
                    if optimise_mask:
                        # update the peak mask
                        opt_peak_mask, opt_non_bg_mask, peak_label = find_peak_mask(signal, ixlo_opt, ixhi_opt, irow,
                                                                                    icol, use_nearest)
                        if not np.any(np.logical_and(opt_peak_mask, det_edges)) or integrate_on_edge:
                            # combine masks as optimal TOF window can truncate peak slightly
                            opt_peak_mask = np.logical_or(peak_mask, opt_peak_mask)
                            is_valid_mask, new_status = is_peak_mask_valid(opt_peak_mask, peak_label, npk_min,
                                                                           density_min, nrow_max, ncol_max,
                                                                           min_npixels_per_vacancy, max_nvacancies)
                            if is_valid_mask:
                                status = new_status
                                # refocus data and re-optimise TOF limits
                                peak_mask = opt_peak_mask
                                non_bg_mask = np.logical_and(non_bg_mask, opt_non_bg_mask)
                                ypk, epk_sq = focus_data_in_detector_mask(signal, error, peak_mask, non_bg_mask, ixpk)
                                ixlo_opt, ixhi_opt = find_peak_limits(ypk, epk_sq, min(ixlo, ixlo_opt),
                                                                      max(ixhi, ixhi_opt))
                    if update_peak_pos:
                        # find det
                        idet = np.argmax(signal[:, :, ixlo_opt:ixhi_opt].sum(axis=2)[peak_mask])
                        det = dets[peak_mask][idet]
                        irow, icol = np.where(dets == det)
                        # update tof
                        imax = np.argmax(ypk[ixlo_opt:ixhi_opt]) + ixlo_opt
                        # replace last added peak
                        irows_delete.append(pk_ws_int.getNumberPeaks() - 1)
                        self.child_AddPeak(PeaksWorkspace=pk_ws_int, RunWorkspace=ws, TOF=xpk[imax],
                                           DetectorID=int(det))
                        pk = pk_ws_int.getPeak(pk_ws_int.getNumberPeaks() - 1)
                    # normalise by bin width before final integration
                    dx = np.diff(xpk)
                    ypk[1:] = ypk[1:] * dx
                    ypk[0] = ypk[0] * dx[0]  # assume first has same dx as adjacent bin
                    epk_sq[1:] = epk_sq[1:] * (dx ** 2)
                    epk_sq[0] = epk_sq[0] * (dx[0] ** 2)
                    # calc intens and sig
                    th = pk.getScattering() / 2
                    wl = pk.getWavelength()
                    L = (np.sin(th) ** 2) / (wl ** 4)  # at updated peak pos if applicable
                    intens = L * np.sum(ypk[ixlo_opt:ixhi_opt])
                    sig = L * np.sqrt(np.sum(epk_sq[ixlo_opt:ixhi_opt]))
                    integrated = True
            # update peak with new intens and sig
            pk.setIntensity(intens)
            pk.setSigmaIntensity(sig)
            if plot_filename:
                if not integrated:
                    ypk = signal[peak_mask].sum(axis=0)
                    epk_sq = np.sum(error[peak_mask] ** 2, axis=0)
                    ixlo_opt = ixlo
                    ixhi_opt = ixhi
                image_data = signal[:, :, ixlo_opt:ixhi_opt].sum(axis=2)
                cbar = plot_integrated_peak(fig, ax, xpk, ypk, epk_sq, image_data, peak_mask, irow, icol, ixlo, ixhi,
                                            ixlo_opt, ixhi_opt, pk, ipk, status, intens, sig, LogNorm)
                pdf.savefig(fig, rasterized=False)
                [subax.clear() for subax in ax]  # clear axes for next figure rather than make new one (quicker)
                cbar.remove()
            # update progress
            prog_reporter.report("Integrating Peaks")
        self.child_DeleteTableRows(TableWorkspace=pk_ws_int, Rows=irows_delete)
        if plot_filename:
            plt.close(fig)
            pdf.close()
        # assign output
        self.setProperty("OutputWorkspace", pk_ws_int)

    def child_CloneWorkspace(self, **kwargs):
        alg = self.createChildAlgorithm("CloneWorkspace", enableLogging=False)
        for prop, value in kwargs.items():
            alg.setProperty(prop, value)
        alg.execute()
        return alg.getProperty("OutputWorkspace").value

    def child_DeleteTableRows(self, **kwargs):
        alg = self.createChildAlgorithm("DeleteTableRows", enableLogging=False)
        for prop, value in kwargs.items():
            alg.setProperty(prop, value)
        alg.execute()

    def child_AddPeak(self, **kwargs):
        alg = self.createChildAlgorithm("AddPeak", enableLogging=False)
        for prop, value in kwargs.items():
            alg.setProperty(prop, value)
        alg.execute()


# register algorithm with mantid
AlgorithmFactory.subscribe(IntegratePeaksSkew)
