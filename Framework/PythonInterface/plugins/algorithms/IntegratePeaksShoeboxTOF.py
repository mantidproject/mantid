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
    Progress,
)
from mantid.kernel import (
    Direction,
    FloatBoundedValidator,
    IntBoundedValidator,
    StringListValidator,
    EnabledWhenProperty,
    PropertyCriterion,
    SpecialCoordinateSystem,
)
from dataclasses import dataclass
import numpy as np
from scipy.ndimage import distance_transform_edt, maximum_position, label
from scipy.signal import convolve
from plugins.algorithms.peakdata_utils import (
    InstrumentArrayConverter,
    get_fwhm_from_back_to_back_params,
    round_up_to_odd_number,
    get_bin_width_at_tof,
    set_peak_intensity,
    make_kernel,
    get_kernel_shape,
)
from enum import Enum
from mantid.dataobjects import PeakShapeDetectorBin


class PEAK_STATUS(Enum):
    WEAK = "Weak peak"
    STRONG = "Strong peak"
    ON_EDGE = "Peak mask is on the detector edge"
    ON_EDGE_INTEGRATED = "Peak mask is on the detector edge"
    NO_PEAK = "No peak detected."


@dataclass
class WeakPeak:
    ipk: int
    ispec: int  # spectrum at center of kernel that corresponds to max I/sigma from convolution
    tof: float  # TOF at center of kernel that corresponds to max I/sigma from convolution
    tof_fwhm: float
    tof_bin_width: float
    ipks_near: np.ndarray
    kernel_shape: tuple


class ShoeboxResult:
    """
    This class is used to hold data and integration parameters for single-crystal Bragg peaks
    """

    def __init__(self, ipk, pk, x, y, peak_shape, ipos, ipk_pos, intens_over_sig, status, strong_peak=None, ipk_strong=None):
        self.peak_shape = list(peak_shape)
        self.kernel_shape = list(get_kernel_shape(*self.peak_shape)[0])
        self.ipos = list(ipos) if ipos is not None else ipos
        self.ipos_pk = list(ipk_pos)
        self.labels = ["Row", "Col", "TOF"]
        self.ysum = []
        self.x = x
        self.xmin, self.xmax = np.round(x[0], 1), np.round(x[-1], 1)
        self.status = status
        # integrate y over each dim
        for idim in range(len(peak_shape)):
            self.ysum.append(y.sum(axis=idim))
        # extract peak properties
        intens_over_sig = np.round(intens_over_sig, 1)
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
        if strong_peak is not None and ipk_strong is not None:
            self.title = f"{self.title} (shoebox taken from ipk={ipk_strong} {np.round(strong_peak.getHKL(), 2)})"

    def plot_integrated_peak(self, fig, axes, norm_func, rect_func):
        for iax, ax in enumerate(axes):
            im = ax.imshow(self.ysum[iax], aspect=self.ysum[iax].shape[1] / self.ysum[iax].shape[0])
            im.set_norm(norm_func())
            # plot shoebox
            if self.ipos is not None:
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
        # overwrite axes tick labels for TOF axes
        nticks = 4
        xticklabels = nticks * [""]
        xticklabels[0] = str(self.xmin)
        xticklabels[-1] = str(self.xmax)
        for ax in axes[:-1]:
            ax.set_xticks(np.linspace(*ax.get_xlim(), nticks))
            ax.set_xticklabels(xticklabels)
        # set title
        fig.suptitle(self.title)

    def plot_shoebox(self, iax, ax, rect_func):
        center_peak_pos = self.ipos.copy()
        center_peak_pos.pop(iax)
        # plot peak region
        shape = self.peak_shape.copy()
        shape.pop(iax)
        self._plot_rect(ax, center_peak_pos, shape, rect_func)
        # plot kernel (incl. bg shell)
        shape = self.kernel_shape.copy()
        shape.pop(iax)
        self._plot_rect(ax, center_peak_pos, shape, rect_func, ls="--")

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
            doc="The output PeaksWorkspace will be a copy of the input PeaksWorkspace with the integrated intensities.",
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
            defaultValue=4.0,
            direction=Direction.Input,
            validator=FloatBoundedValidator(lower=1.0),
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
            validator=IntBoundedValidator(lower=0),
            doc="Shoeboxes containing detectors NRowsEdge from the detector edge are defined as on the edge.",
        )
        self.declareProperty(
            name="NColsEdge",
            defaultValue=1,
            direction=Direction.Input,
            validator=IntBoundedValidator(lower=0),
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
            "Optional file path in which to write diagnostic plots (note this will slow the execution of algorithm).",
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
                issues["GetNBinsFromBackToBackParams"] = (
                    "Workspace doesn't have back to back exponential coefficients defined in the parameters.xml file."
                )
        return issues

    def PyExec(self):
        # get input
        ws = self.getProperty("InputWorkspace").value
        peaks = self.getProperty("PeaksWorkspace").value
        # shoebox dimensions
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
        weak_peaks_list = []
        ipks_strong = []
        results = np.full(peaks.getNumberPeaks(), None)
        peaks_det_ids = np.full(peaks.getNumberPeaks(), None)
        prog_reporter = Progress(self, start=0.0, end=1.0, nreports=peaks.getNumberPeaks())
        for ipk, peak in enumerate(peaks):
            prog_reporter.report("Integrating")
            intens, sigma = 0.0, 0.0

            detid = peak.getDetectorID()
            bank_name = peaks.column("BankName")[ipk]
            pk_tof = peak.getTOF()

            # check TOF is in limits of x-axis
            xdim = ws.getXDimension()
            if xdim.getMinimum() < pk_tof < xdim.getMaximum():
                # get shoebox kernel for initial integration
                nrows = self.getProperty("NRows").value  # get these inside loop as overwritten if shoebox optimised
                ncols = self.getProperty("NCols").value
                ispec = ws.getIndicesFromDetectorIDs([detid])[0]
                bin_width = get_bin_width_at_tof(ws, ispec, pk_tof)  # used later to scale intensity
                if get_nbins_from_b2bexp_params:
                    fwhm = get_fwhm_from_back_to_back_params(peak, ws, detid)
                    nbins = max(3, int(nfwhm * fwhm / bin_width)) if fwhm is not None else self.getProperty("NBins").value
                    nbins = round_up_to_odd_number(nbins)
                else:
                    nbins = self.getProperty("NBins").value
                kernel = make_kernel(nrows, ncols, nbins)

                # get data array and crop
                peak_data = array_converter.get_peak_data(
                    peak, detid, bank_name, nshoebox * kernel.shape[0], nshoebox * kernel.shape[1], nrows_edge, ncols_edge
                )
                x, y, esq, ispecs = get_and_clip_data_arrays(ws, peak_data, pk_tof, kernel, nshoebox)
                ix = np.argmin(abs(x - pk_tof))
                ipos_predicted = [peak_data.irow, peak_data.icol, ix]
                det_edges = peak_data.det_edges if not integrate_on_edge else None
                peaks_det_ids[ipk] = peak_data.detids

                intens, sigma, i_over_sig, status, ipos, nrows, ncols, nbins = integrate_peak(
                    ws,
                    peaks,
                    ipk,
                    kernel,
                    nrows,
                    ncols,
                    nbins,
                    x,
                    y,
                    esq,
                    ispecs,
                    ipos_predicted,
                    det_edges,
                    weak_peak_threshold,
                    do_optimise_shoebox,
                )

                if status == PEAK_STATUS.WEAK and do_optimise_shoebox and weak_peak_strategy == "NearestStrongPeak":
                    # look for possible strong peaks at any TOF in the window (won't know if strong until all pks integrated)
                    ipks_near, _ = find_ipks_in_window(ws, peaks, ispecs, ipk)
                    fwhm = fwhm if get_nbins_from_b2bexp_params else None  # not calculated but not going to be used
                    weak_peaks_list.append(
                        WeakPeak(ipk, ispecs[ipos[0], ipos[1]], x[ipos[-1]], fwhm, bin_width, ipks_near, (nrows, ncols, nbins))
                    )
                else:
                    if status == PEAK_STATUS.STRONG:
                        ipks_strong.append(ipk)
                    results[ipk] = ShoeboxResult(
                        ipk, peak, x, y, [nrows, ncols, nbins], ipos, [peak_data.irow, peak_data.icol, ix], i_over_sig, status
                    )  # use this to get strong peak shoebox dimensions even if no plotting
                # scale summed intensity by bin width to get integrated area
                intens = intens * bin_width
                sigma = sigma * bin_width
            set_peak_intensity(peak, intens, sigma, do_lorz_cor)

        if len(ipks_strong):
            prog_reporter.resetNumSteps(int(len(weak_peaks_list)), start=0.0, end=1.0)
            for weak_pk in weak_peaks_list:
                prog_reporter.report("Re-integrating weak peaks")
                # get peak
                ipk = weak_pk.ipk
                peak = peaks.getPeak(ipk)
                bank_name = peaks.column("BankName")[ipk]
                pk_tof = peak.getTOF()
                # find nearest strong peak to get shoebox dimensions from
                ipk_strong, strong_pk = get_nearest_strong_peak(peaks, peak, results, weak_pk.ipks_near, ipks_strong)
                # get peak shape and make kernel
                nrows, ncols, nbins = results[ipk_strong].peak_shape
                # scale TOF extent
                if get_nbins_from_b2bexp_params:
                    # scale TOF extent by ratio of fwhm
                    strong_pk_fwhm = get_fwhm_from_back_to_back_params(strong_pk, ws, strong_pk.getDetectorID())
                    ratio = weak_pk.tof_fwhm / strong_pk_fwhm
                else:
                    # scale assuming resolution dTOF/TOF = const
                    ratio = pk_tof / strong_pk.getTOF()
                # scale ratio by bin widths at the two TOFs (can be different if log-binning)
                ispec_strong = ws.getIndicesFromDetectorIDs([strong_pk.getDetectorID()])[0]
                ratio = ratio * get_bin_width_at_tof(ws, ispec_strong, strong_pk.getTOF()) / weak_pk.tof_bin_width
                nbins = max(3, round_up_to_odd_number(int(nbins * ratio)))
                kernel = make_kernel(nrows, ncols, nbins)
                # get data array in peak region (keep same window size, nshoebox, for plotting)
                peak_data = array_converter.get_peak_data(
                    peak, peak.getDetectorID(), bank_name, nshoebox * kernel.shape[0], nshoebox * kernel.shape[1], nrows_edge, ncols_edge
                )
                x, y, esq, ispecs = get_and_clip_data_arrays(ws, peak_data, pk_tof, kernel, nshoebox)

                # integrate at previously found ipos
                if weak_pk.ispec not in ispecs:
                    nrows = max(kernel.shape[0], weak_pk.kernel_shape[0])
                    ncols = max(kernel.shape[0], weak_pk.kernel_shape[0])
                    peak_data = array_converter.get_peak_data(
                        peak,
                        peak.getDetectorID(),
                        bank_name,
                        nshoebox * nrows,
                        nshoebox * ncols,
                        nrows_edge,
                        ncols_edge,
                    )
                    x, y, esq, ispecs = get_and_clip_data_arrays(ws, peak_data, pk_tof, kernel, nshoebox)
                ipos = [*np.argwhere(ispecs == weak_pk.ispec)[0], np.argmin(abs(x - weak_pk.tof))]
                peaks_det_ids[ipk] = peak_data.detids

                det_edges = peak_data.det_edges if not integrate_on_edge else None
                intens, sigma, i_over_sig, status, ipos, nrows, ncols, nbins = integrate_peak(
                    ws,
                    peaks,
                    ipk,
                    kernel,
                    nrows,
                    ncols,
                    nbins,
                    x,
                    y,
                    esq,
                    ispecs,
                    ipos,
                    det_edges,
                    weak_peak_threshold,
                    False,
                )

                # scale summed intensity by bin width to get integrated area
                intens = intens * weak_pk.tof_bin_width
                sigma = sigma * weak_pk.tof_bin_width
                set_peak_intensity(peak, intens, sigma, do_lorz_cor)
                if output_file:
                    # save result for plotting
                    peak_shape = [nrows, ncols, nbins]
                    ipos_predicted = [peak_data.irow, peak_data.icol, np.argmin(abs(x - pk_tof))]
                    results[ipk] = ShoeboxResult(
                        ipk, peak, x, y, peak_shape, ipos, ipos_predicted, i_over_sig, status, strong_peak=strong_pk, ipk_strong=ipk_strong
                    )
        elif weak_peak_strategy == "NearestStrongPeak":
            raise ValueError(
                f"No peaks found with I/sigma > WeakPeakThreshold ({weak_peak_threshold}) - can't "
                f"estimate shoebox dimension for weak peaks. Try reducing WeakPeakThreshold or set "
                f"WeakPeakStrategy to Fix"
            )

        # Sets PeakShapeDetectorBin shapes for successfully integrated peaks
        self._set_peak_shapes(results, peaks_det_ids, peaks)

        # plot output
        if output_file:
            prog_reporter.resetNumSteps(int(len(results) - np.sum(results is None)), start=0.0, end=1.0)
            plot_integration_results(output_file, results, prog_reporter)

        # assign output
        self.setProperty("OutputWorkspace", peaks)

    def _set_peak_shapes(self, shoebox_results, peaks_det_ids, peaks_ws):
        """
        Sets PeakShapeDetectorBin shapes for the successfully integrated peaks
        @param shoebox_results - ShoeboxResult array for each peak
        @param peaks_det_ids - detector ids related to each peak
        @param peaks_ws - peaks workspace
        """
        for ipk, (shoebox_res, peak_detids) in enumerate(zip(shoebox_results, peaks_det_ids)):
            if shoebox_res is not None and peak_detids is not None:
                if shoebox_res.status != PEAK_STATUS.ON_EDGE and shoebox_res.status != PEAK_STATUS.NO_PEAK:
                    det_id_row_start = shoebox_res.ipos[0] - shoebox_res.peak_shape[0] // 2
                    det_id_row_end = shoebox_res.ipos[0] + shoebox_res.peak_shape[0] // 2 + 1
                    det_id_col_start = shoebox_res.ipos[1] - shoebox_res.peak_shape[1] // 2
                    det_id_col_end = shoebox_res.ipos[1] + shoebox_res.peak_shape[1] // 2 + 1
                    selected_det_ids = peak_detids[det_id_row_start:det_id_row_end, det_id_col_start:det_id_col_end]
                    xstart = shoebox_res.x[shoebox_res.ipos[2] - shoebox_res.peak_shape[2] // 2]
                    xend = shoebox_res.x[shoebox_res.ipos[2] + shoebox_res.peak_shape[2] // 2]
                    det_bin_list = [(int(det_id), xstart, xend) for det_id in selected_det_ids.ravel()]
                    peak_shape = PeakShapeDetectorBin(det_bin_list, SpecialCoordinateSystem.NONE, self.name(), self.version())
                    peak = peaks_ws.getPeak(ipk)
                    peak.setPeakShape(peak_shape)

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


def get_nearest_strong_peak(peaks, peak, results, ipks_near, ipks_strong):
    ipks_near_strong = []
    for ipk_near in ipks_near:
        if results[ipk_near] and results[ipk_near].status == PEAK_STATUS.STRONG:
            ipks_near_strong.append(int(ipk_near))
    if not ipks_near_strong:
        # no peaks in detector window at any TOF, look in all table
        ipks_near_strong = ipks_strong
    # loop over strong peaks and find nearest peak
    ipk_strong = None
    angle_min = np.inf
    for ipk_near in ipks_near_strong:
        angle = calc_angle_between_peaks(peak, peaks.getPeak(ipk_near))
        if angle < angle_min:
            angle_min = angle
            ipk_strong = ipk_near
    return ipk_strong, peaks.getPeak(ipk_strong)


def integrate_peak(
    ws, peaks, ipk, kernel, nrows, ncols, nbins, x, y, esq, ispecs, ipos_predicted, det_edges, weak_peak_threshold, do_optimise_shoebox
):
    # perform initial integration
    intens_over_sig = convolve_shoebox(y, esq, kernel)  # array of I/sigma same size as data

    # identify best shoebox position near peak
    ipos = find_nearest_peak_in_data_window(intens_over_sig, ispecs, x, ws, peaks, ipk, tuple(ipos_predicted))

    # perform final integration if required
    intens, sigma, i_over_sig = 0.0, 0.0, 0.0
    status = PEAK_STATUS.NO_PEAK
    if ipos is not None:
        # integrate at that position (without smoothing I/sigma)
        intens, sigma, i_over_sig, status = integrate_shoebox_at_pos(y, esq, kernel, ipos, weak_peak_threshold, det_edges)
        if status == PEAK_STATUS.STRONG and do_optimise_shoebox:
            ipos, (nrows, ncols, nbins) = optimise_shoebox(y, esq, (nrows, ncols, nbins), ipos)
            kernel = make_kernel(nrows, ncols, nbins)
            # re-integrate but this time check for overlap with edge
            intens, sigma, i_over_sig, status = integrate_shoebox_at_pos(y, esq, kernel, ipos, weak_peak_threshold, det_edges)
    return intens, sigma, i_over_sig, status, ipos, nrows, ncols, nbins


def plot_integration_results(output_file, results, prog_reporter):
    # import inside this function as not allowed to import at point algorithms are registered
    from matplotlib.pyplot import subplots, close
    from matplotlib.patches import Rectangle
    from matplotlib.colors import LogNorm
    from matplotlib.backends.backend_pdf import PdfPages

    try:
        with PdfPages(output_file) as pdf:
            for result in results:
                if result:
                    prog_reporter.report("Plotting")
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


def calc_angle_between_peaks(pk1, pk2):
    return abs(pk1.getQLabFrame().angle(pk2.getQLabFrame()))


def get_and_clip_data_arrays(ws, peak_data, pk_tof, kernel, nshoebox):
    x, y, esq, ispecs = peak_data.get_data_arrays()  # 3d arrays [rows x cols x tof]
    x = x[peak_data.irow, peak_data.icol, :]  # take x at peak centre, should be same for all detectors
    ispec = ispecs[peak_data.irow, peak_data.icol]
    # crop data array to TOF region of peak using shoebox dimension
    itof = ws.yIndexOfX(pk_tof, int(ispec))  # need index in y now (note x values are points even if were edges)
    tof_slice = slice(
        int(np.clip(itof - nshoebox * kernel.shape[-1] // 2, a_min=0, a_max=len(x))),
        int(np.clip(itof + nshoebox * kernel.shape[-1] // 2, a_min=0, a_max=len(x))),
    )
    x = x[tof_slice]
    y = y[:, :, tof_slice]
    esq = esq[:, :, tof_slice]
    return x, y, esq, ispecs


def convolve_shoebox(y, esq, kernel, mode="same"):
    yconv = convolve(y, kernel, mode=mode)
    with np.errstate(divide="ignore", invalid="ignore"):
        econv = np.sqrt(convolve(esq, kernel**2, mode=mode))
        intens_over_sig = yconv / econv
    intens_over_sig[~np.isfinite(intens_over_sig)] = 0
    # zero edges where convolution is invalid
    if mode == "same":
        edge_mask = np.ones(intens_over_sig.shape, dtype=bool)
        center_slice = tuple(slice(nedge, -nedge) for nedge in (np.asarray(kernel.shape) - 1) // 2)
        edge_mask[center_slice] = False
        intens_over_sig[edge_mask] = 0
    return intens_over_sig


def find_nearest_peak_in_data_window(data, ispecs, x, ws, peaks, ipk, ipos, min_threshold=2):
    # find threshold
    threshold = max(0.5 * (data[ipos] + min_threshold), min_threshold)
    labels, nlabels = label(data > threshold)
    if nlabels < 1:
        return None  # no peak found
    peak_label = labels[ipos]
    if peak_label == 0:
        dists, inearest = distance_transform_edt(labels == 0, return_distances=True, return_indices=True)
        nearest_label = labels[tuple(inearest)]
        # mask out labels closest to other peaks
        ipks_near, ispecs_peaks = find_ipks_in_window(ws, peaks, ispecs, ipk, tof_min=x.min(), tof_max=x.max())
        tofs = peaks.column("TOF")
        for ii, ipk_near in enumerate(ipks_near):
            ipos_near = tuple([*np.argwhere(ispecs == ispecs_peaks[ii])[0], np.argmin(abs(x - tofs[ipk_near]))])
            ilabel_near = nearest_label[ipos_near]
            if ilabel_near != nearest_label[ipos] or dists[ipos] > dists[ipos_near]:
                labels[labels == nearest_label[ipos_near]] = 0  # remove the label
        if not labels.any():
            # no more peak regions left
            return None
        inearest = distance_transform_edt(labels == 0, return_distances=False, return_indices=True)
        peak_label = labels[tuple(inearest)][ipos]
    return maximum_position(data, labels, peak_label)


def find_ipks_in_window(ws, peaks, ispecs, ipk, tof_min=None, tof_max=None):
    ispecs_peaks = np.asarray(ws.getIndicesFromDetectorIDs([int(p.getDetectorID()) for p in peaks]))
    ipks_near = np.isin(ispecs_peaks, ispecs)
    if tof_min and tof_max:
        tofs = peaks.column("TOF")
        ipks_near = np.logical_and.reduce((tofs >= tof_min, tofs <= tof_max, ipks_near))
    ipks_near = np.flatnonzero(ipks_near)  # convert to index
    ipks_near = np.delete(ipks_near, np.where(ipks_near == ipk))  # remove the peak of interest
    return ipks_near, ispecs_peaks[ipks_near]


def integrate_shoebox_at_pos(y, esq, kernel, ipos, weak_peak_threshold, det_edges=None):
    slices = tuple(
        [
            slice(
                np.clip(ii - kernel.shape[idim] // 2, a_min=0, a_max=y.shape[idim]),
                np.clip(ii + kernel.shape[idim] // 2 + 1, a_min=0, a_max=y.shape[idim]),
            )
            for idim, ii in enumerate(ipos)
        ]
    )

    status = PEAK_STATUS.NO_PEAK
    intens, sigma, i_over_sig = 0.0, 0.0, 0.0
    if det_edges is not None and det_edges[slices[:-1]].any():
        status = PEAK_STATUS.ON_EDGE
    else:
        if y[slices].size != kernel.size:
            # peak is partially on edge, but we continue to integrate with a partial kernel
            status = PEAK_STATUS.ON_EDGE_INTEGRATED  # don't want to optimise weak peak shoeboxes using edge peaks
            kernel_slices = []
            for idim in range(len(ipos)):
                # start/stop indices in kernel (by default all elements)
                istart, iend = 2 * [None]
                # get index in y where kernel starts
                iy_start = ipos[idim] - kernel.shape[idim] // 2
                if iy_start < 0:
                    istart = -iy_start  # chop of ii elements at the beginning of the kernel along this dimension
                elif iy_start > y.shape[idim] - kernel.shape[idim]:
                    iend = y.shape[idim] - iy_start  # include only up to number of elements remaining in y
                kernel_slices.append(slice(istart, iend))
            kernel = kernel[tuple(kernel_slices)]
            # re-normalise the shell to have integral = 0
            inegative = kernel > 0
            kernel[inegative] = -np.sum(kernel[~inegative]) / np.sum(inegative)
        intens = np.sum(y[slices] * kernel)
        sigma = np.sqrt(np.sum(esq[slices] * (kernel**2)))
        i_over_sig = intens / sigma if sigma > 0 else 0.0
        if status == PEAK_STATUS.NO_PEAK:
            status = PEAK_STATUS.STRONG if i_over_sig > weak_peak_threshold else PEAK_STATUS.WEAK
    return intens, sigma, i_over_sig, status


def optimise_shoebox(y, esq, peak_shape, ipos, nfail_max=2):
    best_peak_shape = list(peak_shape)
    best_ipos = list(ipos)
    for idim in range(3)[::-1]:
        nfailed = 0
        max_intens_over_sig = -np.inf
        this_peak_shape = best_peak_shape.copy()
        this_peak_shape[idim] = min(1, round_up_to_odd_number(peak_shape[idim] // 2) - 2)
        while True:
            # make kernel
            this_peak_shape[idim] = this_peak_shape[idim] + 2
            kernel = make_kernel(*this_peak_shape)
            # get slice the size of kernel along all other dims centered on best ipos from other dims
            slices = [
                slice(np.clip(ii - kernel.shape[idim] // 2, 0, y.shape[idim]), np.clip(ii + kernel.shape[idim] // 2 + 1, 0, y.shape[idim]))
                for idim, ii in enumerate(best_ipos)
            ]
            # incl. elements along dim that give valid convolution in initial peak region
            slices[idim] = slice(
                np.clip(ipos[idim] - peak_shape[idim] // 2 - kernel.shape[idim] // 2, 0, y.shape[idim]),
                np.clip(ipos[idim] + peak_shape[idim] // 2 + kernel.shape[idim] // 2 + 1, 0, y.shape[idim]),
            )
            slices = tuple(slices)
            if y[slices].shape[idim] <= kernel.shape[idim]:
                # no valid convolution region
                break
            this_intens_over_sig = np.squeeze(convolve_shoebox(y[slices], esq[slices], kernel, mode="valid"))
            imax = np.argmax(this_intens_over_sig)
            if this_intens_over_sig[imax] > max_intens_over_sig:
                max_intens_over_sig = this_intens_over_sig[imax]
                best_ipos[idim] = imax + max(ipos[idim] - peak_shape[idim] // 2, kernel.shape[idim] // 2)
                best_peak_shape[idim] = this_peak_shape[idim]
                nfailed = 0
            else:
                nfailed += 1
                if not nfailed < nfail_max:
                    break
    return best_ipos, best_peak_shape


# register algorithm with mantid
AlgorithmFactory.subscribe(IntegratePeaksShoeboxTOF)
