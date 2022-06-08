# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
from mantid.api import (DataProcessorAlgorithm, AlgorithmFactory, Progress, MatrixWorkspaceProperty,
                        IPeaksWorkspaceProperty, FileProperty, FileAction, WorkspaceUnitValidator)
from mantid.kernel import (Direction, FloatBoundedValidator, IntBoundedValidator)
import numpy as np
from mantid.simpleapi import *
from scipy.signal import convolve2d
from scipy.ndimage import label
from scipy.stats import moment
from mantid.geometry import RectangularDetector, GridDetector
import matplotlib.pyplot as plt


class InstrumentArrayConverter:
    """
    This class is used to convert the spectra in the region of a peak to a 3D numpy array of signal and errors
    Note that it assumes that banks are not adjacent - i.e. it truncates the data to the edge of the bank
    """

    def __init__(self, ws):
        self.ws = ws
        self.inst = ws.getInstrument()
        if any(inst_comp for inst_comp in self.inst if
               isinstance(inst_comp, RectangularDetector) or isinstance(inst_comp, GridDetector)):
            # might not be true for all components due to presence of monitors etc.
            self.get_peak_array = self._get_peak_region_array_rectangular_detector
        else:
            self.get_peak_array = self._get_peak_array_general

    def _get_peak_array_general(self, peak, bank_name):
        # To-Do - expose NearestNeighbourWorkspaceInfo.h to python
        return None

    def get_rectangular_component(self, bank_name):
        return next((inst_comp for inst_comp in self.inst if (
                isinstance(inst_comp, RectangularDetector) or isinstance(inst_comp, GridDetector))
                     and bank_name in inst_comp.getFullName()), None)

    def _get_peak_region_array_rectangular_detector(self, peak, detid, bank_name, dpixel=8):
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
        comp = self.get_rectangular_component(bank_name)
        if comp is None:
            return
        col_step, row_step = comp.idstep(), comp.idstepbyrow()  # step in detID along col and row

        row, col = peak.getRow(), peak.getCol()
        # need to adjust range depending on whether above min/max row/col
        drow_vec = np.arange(max(0, row - dpixel), min(row + dpixel + 1, comp.xpixels())) - row
        dcol_vec = np.arange(max(0, col - dpixel), min(col + dpixel + 1, comp.ypixels())) - col
        dcol, drow = np.meshgrid(dcol_vec, drow_vec)
        detids = detid + dcol * col_step + drow * row_step

        ispecs = np.array(self.ws.getIndicesFromDetectorIDs(
            [int(d) for d in detids.flatten()])).reshape(detids.shape)

        signal = np.zeros((*ispecs.shape, self.ws.blocksize()))
        errors = np.zeros(signal.shape)
        for irow in range(signal.shape[0]):
            for icol in range(signal.shape[1]):
                ispec = int(ispecs[irow, icol])
                signal[irow, icol, :] = self.ws.readY(ispec)
                errors[irow, icol, :] = self.ws.readE(ispec)

        irow_peak = np.where(drow_vec == 0)[0][0]
        icol_peak = np.where(dcol_vec == 0)[0][0]
        ispec = int(ispecs[irow_peak, icol_peak])

        detector_edges = np.logical_or.reduce((drow == -row, dcol == -col,
                                               drow == comp.xpixels() - 1 - row, dcol == comp.ypixels() - 1 - col))

        return signal, errors, irow_peak, icol_peak, ispec, detector_edges


def is_peak_mask_valid(peak_mask, npk_min=3, density_min=0.35, nrow_max=8, ncol_max=15,
                       min_npixels_per_vacancy=2, max_nvacancies=1):
    if peak_mask.sum() < npk_min:
        print('peak_mask.sum() < npk_min')
        return False
    if np.sum(peak_mask.sum(axis=1) > 0) > ncol_max:
        print('peak_mask.sum(axis=1) > 0) > ncol_max')
        return False
    if np.sum(peak_mask.sum(axis=0) > 0) > nrow_max:
        print("np.sum(peak_mask.sum(axis=0) > 0) > nrow_max")
        return False
    density = peak_mask.sum() / (np.sum(peak_mask.sum(axis=0) > 0) * np.sum(peak_mask.sum(axis=1) > 0))
    if density < density_min:
        print("peak_mask.sum() / peak_mask.size < density_min")
        return False
    if does_peak_have_vacancies(peak_mask, min_npixels_per_vacancy, max_nvacancies):
        print('has vacancies')
        return False
    return True


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
    iend = len(signal)
    prev_skew = moment(signal[ibg_seed[isort[:iend]]], 3)
    for istart in range(1, iend):
        this_skew = moment(signal[ibg_seed[isort[istart:iend]]], 3)
        if this_skew >= prev_skew:  # this_skew <= 0 or
            break
        else:
            prev_skew = this_skew
    return ibg_seed[isort[istart:iend]], ibg_seed[isort[:istart]]  # bg, non-bg


def calc_snr(signal, error_sq):
    return np.sum(signal) / np.sqrt(np.sum(error_sq))


def optimise_window_intens_over_sig(signal, error_sq,  ilo, ihi, ntol=8):
    if ihi == ilo:
        ihi += 1
    nbad = 0
    prev_IoverSig = calc_snr(signal[ilo:ihi], error_sq[ilo:ihi])
    for istep_hi in range(1, signal.size - ihi):
        this_IoverSig = calc_snr(signal[ilo:ihi+istep_hi], error_sq[ilo:ihi+istep_hi])
        if this_IoverSig > prev_IoverSig:
            prev_IoverSig = this_IoverSig
            nbad = 0
        else:
            nbad += 1
            if nbad > ntol:
                break
    istep_hi -= nbad
    nbad = 0
    prev_IoverSig = calc_snr(signal[ilo:ihi], error_sq[ilo:ihi])
    for istep_lo in range(1, ilo):
        this_IoverSig = calc_snr(signal[ilo-istep_lo:ihi], error_sq[ilo-istep_lo:ihi])
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
    # find initial background points in tof window using skew method (excl. points above mean)
    ibg, _ = find_bg_pts_seed_skew(signal[ilo:ihi])
    ibg += ilo
    # create mask and find largest contiguous region of peak bins
    skew_mask = np.ones(ihi - ilo, dtype=bool)
    skew_mask[ibg - ilo] = False
    labels, nlabel = label(skew_mask)
    ilabel = np.argmax([np.sum(labels == ilabel) for ilabel in range(1, nlabel + 1)]) + 1
    istart, iend = np.flatnonzero(labels == ilabel)[[0, -1]] + ilo
    # expand window to maximise I/sig (good for when peak not entirely in window)
    return optimise_window_intens_over_sig(signal, error_sq, istart, iend + 1)#


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
    _, ipeak2D = find_bg_pts_seed_skew(signal[:, :, ixlo:ixhi].sum(axis=2).flatten(),
                                       np.arange(np.prod(signal.shape[0:2])))
    non_bg_mask = np.zeros(signal.shape[0:2], dtype=bool)
    non_bg_mask[np.unravel_index(ipeak2D, signal.shape[0:2])] = True
    labeled_array, num_features = label(non_bg_mask)
    # find label corresponding to peak
    peak_label = labeled_array[irow, icol]
    if peak_label == 0 and use_nearest:
        irows, icols = get_nearest_non_masked_index(non_bg_mask, irow, icol)
        # look for label corresponding to max. num peak pixels
        labels = np.unique(labeled_array[irows, icols])
        ilabel = np.argmax([np.sum(labeled_array == lab) for lab in labels])
        peak_label = labels[ilabel]
        # throw warning if distance is greater than half window?
    peak_mask = labeled_array == peak_label
    return peak_mask, non_bg_mask, peak_label


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
               "determined by maximising I/sig for the peak pixels identified using the skew method. "

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
        self.declareProperty(name="NPixels", defaultValue=8, direction=Direction.Input,
                             validator=IntBoundedValidator(lower=3),
                             doc="Length of window on detector in number of pixels")
        self.declareProperty(name="BackscatteringTOFResolution", defaultValue=0.04, direction=Direction.Input,
                             validator=FloatBoundedValidator(lower=0),
                             doc="Fractional TOF width of peaks at backscattering (resolution dominated by moderator "
                                 "contribution, dT0/T0, and uncertainty in path length dL/L which is assumed constant"
                                 "for all pixels).")
        self.declareProperty(name="ThetaWidth", defaultValue=0.04, direction=Direction.Input,
                             validator=FloatBoundedValidator(lower=0),
                             doc="Theta resolution (estimated from width at forward scattering minus contribution "
                                 "from moderator, dT0/T0, and path length dL/L).")
        self.setPropertyGroup("NPixels", "Integration Window Parameters")
        self.setPropertyGroup("BackscatteringTOFResolution", "Integration Window Parameters")
        self.setPropertyGroup("ThetaWidth", "Integration Window Parameters")
        # peak mask validators
        self.declareProperty(name="IntegrateIfOnEdge", defaultValue=False, direction=Direction.Input,
                             doc="Integrate peaks if contains pixels on edge of the detector.")
        self.declareProperty(name="UseNearestPeak", defaultValue=False, direction=Direction.Input,
                             doc="Find nearest peak pixel if peak position is in a background pixel.")
        self.declareProperty(name="NPixMin", defaultValue=3, direction=Direction.Input,
                             validator=IntBoundedValidator(lower=1),
                             doc="Minimum number of pixels contributing to a peak")
        self.declareProperty(name="DensityPixMin", defaultValue=0.35, direction=Direction.Input,
                             validator=FloatBoundedValidator(lower=0),
                             doc="Minimum density of peak pixels in bounding box")
        self.declareProperty(name="NRowMax", defaultValue=15, direction=Direction.Input,
                             validator=IntBoundedValidator(lower=1),
                             doc="Maximum number of rows in peak mask (note on WISH rows are equivalent to tubes).")
        self.declareProperty(name="NColMax", defaultValue=15, direction=Direction.Input,
                             validator=IntBoundedValidator(lower=1),
                             doc="Maximum number of columns in peak mask")
        self.declareProperty(name="NVacanciesMax", defaultValue=0, direction=Direction.Input,
                             validator=IntBoundedValidator(lower=0),
                             doc="Maximum number of vacancies (contiguous regions of non-peak pixels entirely "
                                 "contained within the peak mask) for a valid peak.")
        self.declareProperty(name="NPixPerVacancyMin", defaultValue=1, direction=Direction.Input,
                             validator=IntBoundedValidator(lower=0),
                             doc="Minimum number of pixels in a vacancy")
        self.setPropertyGroup("IntegrateIfOnEdge", "Peak Mask Validation")
        self.setPropertyGroup("UseNearestPeak", "Peak Mask Validation")
        self.setPropertyGroup("NPixMin", "Peak Mask Validation")
        self.setPropertyGroup("DensityPixMin", "Peak Mask Validation")
        self.setPropertyGroup("NRowMax", "Peak Mask Validation")
        self.setPropertyGroup("NColMax", "Peak Mask Validation")
        self.setPropertyGroup("NVacanciesMax", "Peak Mask Validation")
        self.setPropertyGroup("NPixPerVacancyMin", "Peak Mask Validation")

        # plotting
        self.declareProperty(FileProperty("OutputFile", "", FileAction.Save, ".pdf"),
                             "Optional file path in which to write diagnostic plots (note this will slow the "
                             "execution of algorithm).")
        self.setPropertyGroup("OutputFile", "Plotting")

        # Output
        self.declareProperty(IPeaksWorkspaceProperty(name="OutputWorkspace",
                                                     defaultValue="",
                                                     direction=Direction.Output),
                             doc="The output PeaksWorkspace will be a copy of the input PeaksWorkspace with the"
                                 " integrated intensities.")

    def PyExec(self):
        # get input
        ws = self.getProperty("InputWorkspace").value
        pk_ws = self.getProperty("PeaksWorkspace").value
        # peak window parameters
        dt0_over_t0 = self.getProperty("BackscatteringTOFResolution").value
        dth = self.getProperty("ThetaWidth").value
        dpixel = self.getProperty("NPixels").value
        integrate_on_edge = self.getProperty("IntegrateIfOnEdge").value
        use_nearest = self.getProperty("UseNearestPeak").value
        # peak mask validation
        npk_min = self.getProperty("NPixMin").value
        density_min = self.getProperty("DensityPixMin").value
        nrow_max = self.getProperty("NRowMax").value
        ncol_max = self.getProperty("NColMax").value
        max_nvacancies = self.getProperty("NVacanciesMax").value
        min_npixels_per_vacancy = self.getProperty("NPixPerVacancyMin").value
        # plotting
        plot_filename = self.getProperty("OutputFile").value

        array_converter = InstrumentArrayConverter(ws)

        # setup progress bar
        prog_reporter = Progress(self, start=0.0, end=1.0, nreports=pk_ws.getNumberPeaks())

        # Empty table workspace
        pk_ws_int = CloneWorkspace(InputWorkspace=pk_ws, OutputWorkspace=self.getPropertyValue("OutputWorkspace"))
        # get spectrum indices for all peaks in table
        detids = pk_ws_int.column('DetID')
        bank_names = pk_ws_int.column('BankName')
        tofs = pk_ws_int.column('TOF')
        for ipk, pk in enumerate(pk_ws_int):
            # get data array in window of side length dpixel around peak region (truncated at edge pf detector)
            signal, error, irow, icol, ispec, detector_edges = array_converter.get_peak_array(pk, detids[ipk],
                                                                                              bank_names[ipk], dpixel)
            # get xdata
            xpk = ws.readX(ispec)
            if len(xpk) > ws.blocksize():
                xpk = 0.5 * (xpk[:-1] + xpk[1:])  # convert to bin centers - must be easier way!
            # get TOF window using resolution parameters
            dTOF = tofs[ipk] * np.sqrt(dt0_over_t0 ** 2 + (dth / np.tan(pk.getScattering() / 2)) ** 2)
            ixlo = np.argmin(abs(xpk - (tofs[ipk] - 0.5 * dTOF)))
            ixhi = np.argmin(abs(xpk - (tofs[ipk] + 0.5 * dTOF)))
            ixpk = np.argmin(abs(xpk - tofs[ipk]))
            # find 2D peak in detector window
            peak_mask, non_bg_mask, peak_label = find_peak_mask(signal, ixlo, ixhi, irow, icol, use_nearest)
            intens, sig = 0.0, 0.0
            integrated = False
            if not integrate_on_edge and np.any(np.logical_and(peak_mask, detector_edges)):
                # warn peak on detector
                pass
            elif peak_label == 0 or not is_peak_mask_valid(peak_mask, npk_min, density_min, nrow_max, ncol_max,
                                                           min_npixels_per_vacancy, max_nvacancies):
                # warn peak not found
                pass
            else:
                ypk, epk_sq = focus_data_in_detector_mask(signal, error, peak_mask, non_bg_mask, ixpk)
                # normalise by bin width
                dx = np.diff(xpk)
                dx = np.hstack((dx[0], dx))  # assume first has same dx as adjacent bin
                ypk = ypk * dx
                epk_sq = epk_sq * (dx ** 2)
                # get lorentz factor for subsequent correction
                th = pk.getScattering() / 2
                wl = pk.getWavelength()
                L = (np.sin(th) ** 2) / (wl ** 4)
                # find bg and pk bins in focused spectrum by maximising I/sig
                ixlo_opt, ixhi_opt = find_peak_limits(ypk, epk_sq, ixlo, ixhi)
                intens = L * np.sum(ypk[ixlo_opt:ixhi_opt])
                sig = L * np.sqrt(np.sum(epk_sq[ixlo_opt:ixhi_opt]))
                integrated = True
            # update peak with new intens and sig
            pk.setIntensity(intens)
            pk.setSigmaIntensity(sig)
            if plot_filename:
                fig, ax = plt.subplots(1, 2, subplot_kw={'projection': 'mantid'})
                # 2D plot
                image_data = signal[:, :, ixlo:ixhi].sum(axis=2)
                vmax = 0.5 * (image_data.min() + image_data[peak_mask].mean())
                img = ax[0].imshow(image_data, vmax=vmax)
                ax[0].plot(*np.where(peak_mask.T), 'xw')
                ax[0].plot(icol, irow, 'or')
                title_str = f"{ipk} ({str(pk.getIntHKL())[1:-1]}) lambda={np.round(pk.getWavelength(), 2)} Ang"
                ax[0].set_title(title_str)
                # 1D plot
                ipad = int((ixhi - ixlo) / 2)  # extra portion of data shown outside the 1D window
                if not integrated:
                    ypk = signal[peak_mask].sum(axis=0)
                    epk_sq = np.sum(error[peak_mask] ** 2, axis=0)
                    istart = max(ixlo - ipad, 0)
                    iend = min(ixhi + ipad, len(xpk) - 1)
                else:
                    ax[1].axvline(xpk[ixlo_opt], ls='--', color='b', label='Optimal window')
                    ax[1].axvline(xpk[ixhi_opt], ls='--', color='b')
                    istart = max(min(ixlo, ixlo_opt) - ipad, 0)
                    iend = min(max(ixhi, ixhi_opt) + ipad, len(xpk) - 1)
                ax[1].errorbar(xpk[istart:iend], ypk[istart:iend], yerr=np.sqrt(epk_sq[istart:iend]),
                               marker='o', markersize=3, capsize=2, ls='', color='k', label='data')
                ax[1].axvline(tofs[ipk], ls='--', color='k', label='Centre')
                ax[1].axvline(xpk[ixlo], ls=':', color='r', label='Initial window')
                ax[1].axvline(xpk[ixhi], ls=':', color='r')
                ax[1].axhline(0, ls=':', color='k')
                ax[1].legend(fontsize=7)
                # figure formatting
                intens_over_sig = round(intens / sig, 2) if sig > 0 else 0
                ax[1].set_title(f'I/sig = {intens_over_sig}')
                fig.colorbar(img, orientation='horizontal', ax=ax[0])
                fig.tight_layout()
                fig.show()
            # update progress
            prog_reporter.report("Integrating Peaks")
        # assign output
        self.setProperty("OutputWorkspace", pk_ws_int)


# register algorithm with mantid
AlgorithmFactory.subscribe(IntegratePeaksSkew)
