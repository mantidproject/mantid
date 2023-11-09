# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2023 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
from mantid.api import (
    DataProcessorAlgorithm,
    AlgorithmFactory,
    MatrixWorkspaceProperty,
    IPeaksWorkspaceProperty,
    WorkspaceUnitValidator,
    FileProperty,
    FileAction,
)
from mantid.kernel import (
    Direction,
    FloatBoundedValidator,
    IntBoundedValidator,
    StringListValidator,
    EnabledWhenProperty,
    PropertyCriterion,
)
import numpy as np
from scipy.ndimage import convolve
from IntegratePeaksSkew import InstrumentArrayConverter, get_fwhm_from_back_to_back_params
from FindSXPeaksConvolve import make_kernel, get_kernel_shape
from enum import Enum


class PEAK_STATUS(Enum):
    WEAK = "Weak peak"
    STRONG = "Strong peak"
    ON_EDGE = "Peak mask is on the detector edge"
    NO_PEAK = "No peak detected."


class ShoeboxResult:
    """
    This class is used to hold data and integration parameters for single-crystal Bragg peaks
    """

    def __init__(self, ipk, pk, x, y, peak_shape, ipos, ipk_pos, status):
        self.peak_shape = list(peak_shape)
        self.kernel_shape = list(get_kernel_shape(*self.peak_shape)[0])
        self.ipos = list(ipos)
        self.ipos_pk = list(ipk_pos)
        self.labels = ["Row", "Col", "TOF"]
        self.ysum = []
        self.extents = []
        # integrate y over each dim
        for idim in range(len(peak_shape)):
            self.ysum.append(y.sum(axis=idim))
            if idim < 2:
                self.extents.append([0, y.shape[idim]])
            else:
                self.extents.append([x[0], x[-1]])
        # extract peak properties
        intens_over_sig = np.round(pk.getIntensityOverSigma(), 1)
        hkl = np.round(pk.getHKL(), 2)
        wl = np.round(pk.getWavelength(), 2)
        tth = np.round(np.degrees(pk.getScattering()), 1)
        d = np.round(pk.getDSpacing(), 2)
        self.title = (
            f"{ipk} ({','.join(str(hkl)[1:-1].split())})"
            f"\n$I/\\sigma$={intens_over_sig}\n"
            rf"$\lambda$={wl} $\AA$; "
            rf"$2\theta={tth}^\circ$; "
            rf"d={d} $\AA$"
            f"\n{status.value}"
        )

    def plot_integrated_peak(self, fig, axes, norm_func, rect_func):
        for iax, ax in enumerate(axes):
            extents = self.extents.copy()
            extents.pop(iax)
            im = ax.imshow(
                self.ysum[iax], aspect=self.ysum[iax].shape[1] / self.ysum[iax].shape[0]
            )  # , extent=np.flip(extents, axis=0).flatten())
            im.set_norm(norm_func())
            # plot shoebox
            self.plot_shoebox(iax, ax, rect_func)
            # plot peak position
            cen = self.ipos_pk.copy()
            cen.pop(iax)
            ax.plot(cen[1], cen[0], "xr")
            # set labels
            labels = self.labels.copy()
            labels.pop(iax)
            ax.set_xlabel(labels[1])
            ax.set_ylabel(labels[0])
        # set title
        fig.suptitle(self.title)

    def plot_shoebox(self, iax, ax, rect_func):
        cen = self.ipos.copy()
        cen.pop(iax)
        # plot peak region
        shape = self.peak_shape.copy()
        shape.pop(iax)
        self._plot_rect(ax, cen, shape, rect_func)
        # plot kernel (incl. bg shell)
        shape = self.kernel_shape.copy()
        shape.pop(iax)
        self._plot_rect(ax, cen, shape, rect_func, ls="--")

    def _plot_rect(self, ax, cen, shape, rect_func, ls="-"):
        bottom_left = [cen[1] - shape[1] // 2 - 0.5, cen[0] - shape[0] // 2 - 0.5]
        rect = rect_func(bottom_left, shape[1], shape[0], linewidth=1.5, edgecolor="r", facecolor="none", alpha=0.75, ls=ls)
        ax.add_patch(rect)


class IntegratePeaksShoeboxTOF(DataProcessorAlgorithm):
    def name(self):
        return "IntegratePeaksShoeboxTOF"

    def category(self):
        return "Diffraction\\Reduction"

    def seeAlso(self):
        return ["IntegratePeaksSkew", "FindSXPeaksConvolve"]

    def summary(self):
        return "Integrate single-crystal Bragg peaks in MatrixWorkspaces with x-unit of TOF using a shoebox."

    def PyInit(self):
        # Input
        self.declareProperty(
            MatrixWorkspaceProperty(
                name="InputWorkspace", defaultValue="", direction=Direction.Input, validator=WorkspaceUnitValidator("TOF")
            ),
            doc="A MatrixWorkspace to integrate (x-axis must be TOF).",
        )
        self.declareProperty(
            IPeaksWorkspaceProperty(name="PeaksWorkspace", defaultValue="", direction=Direction.Input),
            doc="A PeaksWorkspace containing the peaks to integrate.",
        )
        self.declareProperty(
            IPeaksWorkspaceProperty(name="OutputWorkspace", defaultValue="", direction=Direction.Output),
            doc="The output PeaksWorkspace will be a copy of the input PeaksWorkspace with the" " integrated intensities.",
        )
        # shoebox dimensions
        self.declareProperty(
            name="NRows",
            defaultValue=5,
            direction=Direction.Input,
            validator=IntBoundedValidator(lower=3),
            doc="Number of row components in the detector to use in the convolution kernel. "
            "For WISH row components correspond to pixels along a single tube.",
        )
        self.declareProperty(
            name="NCols",
            defaultValue=5,
            direction=Direction.Input,
            validator=IntBoundedValidator(lower=3),
            doc="Number of column components in the detector to use in the convolution kernel. "
            "For WISH column components correspond to tubes.",
        )
        self.declareProperty(
            name="NBins",
            defaultValue=11,
            direction=Direction.Input,
            validator=IntBoundedValidator(lower=3),
            doc="Number of TOF bins to use in the convolution kernel.",
        )
        self.declareProperty(
            name="GetNBinsFromBackToBackParams",
            defaultValue=False,
            direction=Direction.Input,
            doc="If true the number of TOF bins used in the convolution kernel will be calculated from the FWHM of the "
            "BackToBackExponential peak using parameters defined in the instrument parameters.xml file.",
        )
        self.declareProperty(
            name="NFWHM",
            defaultValue=4,
            direction=Direction.Input,
            validator=IntBoundedValidator(lower=1),
            doc="If GetNBinsFromBackToBackParams=True then the number of TOF bins will be NFWHM x FWHM of the "
            "BackToBackExponential at the peak detector and TOF.",
        )
        use_nBins = EnabledWhenProperty("GetNBinsFromBackToBackParams", PropertyCriterion.IsDefault)
        self.setPropertySettings("NBins", use_nBins)
        use_nfwhm = EnabledWhenProperty("GetNBinsFromBackToBackParams", PropertyCriterion.IsNotDefault)
        self.setPropertySettings("NFWHM", use_nfwhm)
        self.declareProperty(
            name="NShoeboxInWindow",
            defaultValue=3.0,
            direction=Direction.Input,
            validator=FloatBoundedValidator(lower=2.0),
            doc="Extent of window in TOF and detector row and column as a multiple of the shoebox length along each dimension.",
        )
        for prop in ["NRows", "NCols", "NBins", "GetNBinsFromBackToBackParams", "NFWHM", "NShoeboxInWindow"]:
            self.setPropertyGroup(prop, "Shoebox Dimensions")
        # shoebox optimisation
        self.declareProperty(
            name="OptimiseShoebox",
            defaultValue=True,
            direction=Direction.Input,
            doc="If OptimiseShoebox=True then shoebox size will be optimised to maximise Intensity/Sigma of the peak.",
        )
        self.declareProperty(
            name="WeakPeakStrategy",
            defaultValue="Fix",
            direction=Direction.Input,
            validator=StringListValidator(["Fix", "NearestStrongPeak"]),
            doc="If WeakPeakStrategy=Fix then fix the shoebox dimensions. If WeakPeakStrategy=NearestStrongPeak then"
            "the shoebox dimensions will be taken from the nearest strong peak (determined by StrongPeakThreshold)."
            "The TOF extent of the shoebox will be scaled by the FWHM of the BackToBackExponential peaks if"
            "GetNBinsFromBackToBackParams=True.",
        )
        self.declareProperty(
            name="WeakPeakThreshold",
            defaultValue=0.0,
            direction=Direction.Input,
            validator=FloatBoundedValidator(lower=0.0),
            doc="Intenisty/Sigma threshold below which a peak is considered weak.",
        )
        optimise_shoebox_enabled = EnabledWhenProperty("OptimiseShoebox", PropertyCriterion.IsDefault)
        self.setPropertySettings("WeakPeakStrategy", optimise_shoebox_enabled)
        self.setPropertySettings("WeakPeakThreshold", optimise_shoebox_enabled)
        for prop in ["OptimiseShoebox", "WeakPeakStrategy", "WeakPeakThreshold"]:
            self.setPropertyGroup(prop, "Shoebox Optimisation")
        # peak validation
        self.declareProperty(
            name="IntegrateIfOnEdge",
            defaultValue=False,
            direction=Direction.Input,
            doc="If IntegrateIfOnEdge=False then peaks with shoebox that includes detector IDs at the edge of the bank "
            " will not be integrated.",
        )
        self.declareProperty(
            name="NRowsEdge",
            defaultValue=1,
            direction=Direction.Input,
            validator=IntBoundedValidator(lower=1),
            doc="Shoeboxes containing detectors NRowsEdge from the detector edge are defined as on the edge.",
        )
        self.declareProperty(
            name="NColsEdge",
            defaultValue=1,
            direction=Direction.Input,
            validator=IntBoundedValidator(lower=1),
            doc="Shoeboxes containing detectors NColsEdge from the detector edge are defined as on the edge.",
        )
        edge_check_enabled = EnabledWhenProperty("IntegrateIfOnEdge", PropertyCriterion.IsDefault)
        self.setPropertySettings("NRowsEdge", edge_check_enabled)
        self.setPropertySettings("NColsEdge", edge_check_enabled)
        for prop in ["IntegrateIfOnEdge", "NRowsEdge", "NColsEdge"]:
            self.setPropertyGroup(prop, "Edge Checking")
        # Corrections
        self.declareProperty(
            name="LorentzCorrection",
            defaultValue=True,
            direction=Direction.Input,
            doc="Correct the integrated intensity by multiplying by the Lorentz factor "
            "sin(theta)^2 / lambda^4 - do not do this if the data have already been corrected.",
        )
        self.setPropertyGroup("LorentzCorrection", "Corrections")
        # plotting
        self.declareProperty(
            FileProperty("OutputFile", "", FileAction.OptionalSave, ".pdf"),
            "Optional file path in which to write diagnostic plots (note this will slow the " "execution of algorithm).",
        )
        self.setPropertyGroup("OutputFile", "Plotting")

    def validateInputs(self):
        issues = dict()
        # check shoebox dimensions
        for prop in ["NRows", "NCols", "NBins"]:
            if not self.getProperty(prop).value % 2:
                issues[prop] = f"{prop} must be an odd number."
        # check valid peak workspace
        ws = self.getProperty("InputWorkspace").value
        inst = ws.getInstrument()
        pk_ws = self.getProperty("PeaksWorkspace").value
        if inst.getName() != pk_ws.getInstrument().getName():
            issues["PeaksWorkspace"] = "PeaksWorkspace must have same instrument as the InputWorkspace."
        if pk_ws.getNumberPeaks() < 1:
            issues["PeaksWorkspace"] = "PeaksWorkspace must have at least 1 peak."
        # check that is getting dTOF from back-to-back params then they are present in instrument
        if self.getProperty("GetNBinsFromBackToBackParams").value:
            # check at least first peak in workspace has back to back params
            if not inst.getComponentByName(pk_ws.column("BankName")[0]).hasParameter("B"):
                issues[
                    "GetNBinsFromBackToBackParams"
                ] = "Workspace doesn't have back to back exponential coefficients defined in the parameters.xml file."
        return issues

    def PyExec(self):
        # get input
        ws = self.getProperty("InputWorkspace").value
        peaks = self.getProperty("PeaksWorkspace").value
        # shoebox dimensions
        nrows = self.getProperty("NRows").value
        ncols = self.getProperty("NCols").value
        get_nbins_from_b2bexp_params = self.getProperty("GetNBinsFromBackToBackParams").value
        nfwhm = self.getProperty("NFWHM").value
        nshoebox = self.getProperty("NShoeboxInWindow").value
        # shoebox optimisation
        do_optimise_shoebox = self.getProperty("OptimiseShoebox").value
        weak_peak_strategy = self.getProperty("WeakPeakStrategy").value
        weak_peak_threshold = self.getProperty("WeakPeakThreshold").value
        # validation
        integrate_on_edge = self.getProperty("IntegrateIfOnEdge").value
        nrows_edge = self.getProperty("NRowsEdge").value
        ncols_edge = self.getProperty("NColsEdge").value
        # corrections
        do_lorz_cor = self.getProperty("LorentzCorrection").value
        # saving file
        output_file = self.getProperty("OutputFile").value

        # create output table workspace
        peaks = self.exec_child_alg("CloneWorkspace", InputWorkspace=peaks, OutputWorkspace="out_peaks")

        array_converter = InstrumentArrayConverter(ws)
        ipk_weak = []
        results = np.empty(peaks.getNumberPeaks(), dtype=ShoeboxResult)
        for ipk, peak in enumerate(peaks):
            detid = peak.getDetectorID()
            bank_name = peaks.column("BankName")[ipk]
            pk_tof = peak.getTOF()

            # get shoebox kernel for initial integration
            ispec = ws.getIndicesFromDetectorIDs([detid])[0]
            itof = ws.binIndexOf(pk_tof, ispec)
            bin_width = np.diff(ws.readX(ispec)[itof : itof + 2])[0]  # used later to scale intensity
            if get_nbins_from_b2bexp_params:
                fwhm = get_fwhm_from_back_to_back_params(peak, ws, detid)

                nbins = max(3, int(nfwhm * fwhm / bin_width)) if fwhm is not None else self.getProperty("NBins").value
                if not nbins % 2:
                    nbins += 1  # force to be odd
            else:
                nbins = self.getProperty("NBins").value
            kernel = make_kernel(nrows, ncols, nbins)

            # get data array and crop
            peak_data = array_converter.get_peak_data(
                peak, detid, bank_name, nshoebox * kernel.shape[0], nshoebox * kernel.shape[1], nrows_edge, ncols_edge
            )
            x, y, esq, ispecs = peak_data.get_data_arrays()  # 3d arrays [rows x cols x tof]
            x = x[peak_data.irow, peak_data.icol, :]  # take x at peak centre, should be same for all detectors

            # crop data array to TOF region of peak using shoebox dimension
            itof = ws.yIndexOfX(pk_tof, ispec)  # need index in y now (note x values are points even if were edges)
            tof_slice = slice(
                int(np.clip(itof - nshoebox * kernel.shape[-1] // 2, a_min=0, a_max=len(x))),
                int(np.clip(itof + nshoebox * kernel.shape[-1] // 2, a_min=0, a_max=len(x))),
            )

            x = x[tof_slice]
            y = y[:, :, tof_slice]
            esq = esq[:, :, tof_slice]

            # perform initial integration
            intens_over_sig = convolve_shoebox(y, esq, kernel)

            # identify best shoebox position near peak
            ix = np.argmin(abs(x - pk_tof))
            ipos = find_nearest_peak_in_data(intens_over_sig, ispecs, x, ws, peaks, ipk, peak_data.irow, peak_data.icol, ix)

            # perform final integration if required
            det_edges = peak_data.det_edges if not integrate_on_edge else None
            if ipos is None:
                status = PEAK_STATUS.NO_PEAK
                intens, sigma = 0.0, 0.0
            else:
                # integrate at that position (without smoothing I/sigma)
                intens, sigma = integrate_shoebox_at_pos(y, esq, kernel, ipos, det_edges)
                if np.isfinite(sigma):
                    status = PEAK_STATUS.STRONG if intens / sigma > weak_peak_threshold else PEAK_STATUS.WEAK
                    if status == PEAK_STATUS.STRONG and do_optimise_shoebox:
                        kernel, (nrows, ncols, nbins) = optimise_shoebox(y, esq, kernel.shape, ipos)
                        # re-integrate but this time check for overlap with edge
                        intens, sigma = integrate_shoebox_at_pos(y, esq, kernel, ipos, det_edges)
                else:
                    status = PEAK_STATUS.ON_EDGE
                    intens, sigma = 0.0, 0.0

            if status == PEAK_STATUS.WEAK and do_optimise_shoebox and weak_peak_strategy == "NearestStrongPeak":
                ipk_weak.append(ipk)  # store pk index (and pos FWHM?)
            else:
                set_peak_intensity(peak, intens, sigma, bin_width, do_lorz_cor)
                if output_file:
                    # save result for plotting
                    results[ipk] = ShoeboxResult(ipk, peak, x, y, [nrows, ncols, nbins], ipos, [peak_data.irow, peak_data.icol, ix], status)
        # loop over weak peaks
        #   look for peaks with detectors in ispec window
        #   if no peaks found
        #   look for nearest strong peak

        # plot output
        if output_file:
            from matplotlib.pyplot import subplots, close
            from matplotlib.patches import Rectangle
            from matplotlib.colors import LogNorm
            from matplotlib.backends.backend_pdf import PdfPages

            try:
                with PdfPages(output_file) as pdf:
                    for result in results:
                        fig, axes = subplots(1, 3, figsize=(12, 5), subplot_kw={"projection": "mantid"})
                        fig.subplots_adjust(wspace=0.3)  # ensure plenty space between subplots (want to avoid slow tight_layout)
                        result.plot_integrated_peak(fig, axes, LogNorm, Rectangle)
                        pdf.savefig(fig)
                        close(fig)
            except OSError:
                raise RuntimeError(
                    f"OutputFile ({output_file}) could not be opened - please check it is not open by "
                    f"another programme and that the user has permission to write to that directory."
                )

        # assign output
        self.setProperty("OutputWorkspace", peaks)

    def exec_child_alg(self, alg_name, **kwargs):
        alg = self.createChildAlgorithm(alg_name, enableLogging=False)
        alg.initialize()
        for prop, value in kwargs.items():
            alg.setProperty(prop, value)
        alg.execute()
        if "OutputWorkspace" in alg.outputProperties():
            return alg.getProperty("OutputWorkspace").value
        else:
            return None


def convolve_shoebox(y, esq, kernel):
    yconv = convolve(input=y, weights=kernel, mode="nearest")
    econv = np.sqrt(convolve(input=esq, weights=kernel**2, mode="nearest"))
    with np.errstate(divide="ignore", invalid="ignore"):
        intens_over_sig = yconv / econv
    intens_over_sig[~np.isfinite(intens_over_sig)] = 0
    intens_over_sig = convolve(intens_over_sig, weights=np.ones(3 * [3]) / (3**3), mode="constant")  # 0 pad
    return intens_over_sig


def find_nearest_peak_in_data(data, ispecs, x, ws, peaks, ipk, irow, icol, ix):
    tofs = peaks.column("TOF")
    ispecs_peaks = ws.getIndicesFromDetectorIDs([int(p.getDetectorID()) for p in peaks])
    ipks_near = np.flatnonzero(np.logical_and.reduce((tofs >= x.min(), tofs <= x.max(), np.isin(ispecs_peaks, ispecs))))
    ipks_near = np.delete(ipks_near, np.where(ipks_near == ipk))  # remove the peak of interest
    if len(ipks_near) > 0:
        # get position of nearby peaks in data array in fractional coords
        shape = np.array(data.shape)
        pos_near = [np.array(*np.where(ispecs == ispecs_peaks[ii]), np.array(np.argmin(x - tofs[ii]))) / shape for ii in ipks_near]
        # sort data in descending order and select strongest peak nearest to pk (in fractional coordinates)
        isort = list(zip(*np.unravel_index(np.argsort(-data, axis=None), data.shape)))
        pk_pos = np.array([irow, icol, ix]) / np.array(data.shape)
        imax_nearest = None
        for ibin in isort:
            # calc distance to pk pos
            bin_pos = np.array(ibin) / shape
            pk_dist_sq = np.sum(pk_pos - bin_pos)
            for pos in pos_near:
                if np.sum(pos - bin_pos) > pk_dist_sq:
                    imax_nearest = ibin
                    break
            else:
                continue  # executed if inner loop did not break (i.e. pixel closer to nearby peak than this peak)
            break  # execute if inner loop did break and else branch ignored (i.e. found bin closest to this peak)
        return imax_nearest  # could be None if no peak found
    else:
        # no nearby peaks - return position of maximum in data
        return np.unravel_index(np.argmax(data), data.shape)


def integrate_shoebox_at_pos(y, esq, kernel, ipos, det_edges=None):
    slices = tuple([slice(ii - shape // 2, ii + shape // 2 + 1) for ii, shape in zip(ipos, kernel.shape)])
    intens, sigma = 2 * [np.nan]
    if det_edges is None or not det_edges[slices[:-1]].any():
        intens = np.sum(y[slices] * kernel)
        sigma = np.sqrt(np.sum(esq[slices] * (kernel**2)))
    return intens, sigma


def optimise_shoebox(y, esq, current_shape, ipos):
    # find limits of kernel size from data size
    lengths = []
    for idim, length in enumerate(y.shape):
        max_length = 2 * min(ipos[idim], length - ipos[idim]) + 1
        min_length = max(3, current_shape[idim] // 2)
        if not min_length % 2:
            min_length += 1  # force odd
        lengths.append(np.arange(min_length, max_length - 2 * max(1, max_length // 16), 2))
    # loop over all possible kernel sizes
    best_peak_shape = None
    best_kernel = None
    best_i_over_sig = -np.inf  # null value
    for nrows in lengths[0]:
        for ncols in lengths[1]:
            for nbins in lengths[2]:
                kernel = make_kernel(nrows, ncols, nbins)
                intens, sigma = integrate_shoebox_at_pos(y, esq, kernel, ipos)
                i_over_sig = intens / sigma
                if i_over_sig > best_i_over_sig:
                    best_i_over_sig = i_over_sig
                    best_peak_shape = (nrows, ncols, nbins)
                    best_kernel = kernel
    return best_kernel, best_peak_shape


def set_peak_intensity(pk, intens, sigma, bin_width, do_lorz_cor):
    if do_lorz_cor:
        L = (np.sin(pk.getScattering() / 2) ** 2) / (pk.getWavelength() ** 4)  # at updated peak pos
    else:
        L = 1
    # set peak object intensity
    pk.setIntensity(L * intens * bin_width)
    pk.setSigmaIntensity(L * sigma * bin_width)


# register algorithm with mantid
AlgorithmFactory.subscribe(IntegratePeaksShoeboxTOF)
