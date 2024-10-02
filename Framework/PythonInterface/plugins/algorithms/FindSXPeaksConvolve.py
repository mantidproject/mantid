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
    Progress,
)
from mantid.kernel import (
    Direction,
    FloatBoundedValidator,
    IntBoundedValidator,
    EnabledWhenProperty,
    PropertyCriterion,
    logger,
    StringListValidator,
)
import numpy as np
from scipy.ndimage import label, maximum_position, binary_closing, sum_labels, uniform_filter1d, uniform_filter
from scipy.signal import convolve
from plugins.algorithms.IntegratePeaksSkew import InstrumentArrayConverter, get_fwhm_from_back_to_back_params


class FindSXPeaksConvolve(DataProcessorAlgorithm):
    def name(self):
        return "FindSXPeaksConvolve"

    def category(self):
        return "Diffraction\\Reduction"

    def seeAlso(self):
        return ["FindSXPeaks"]

    def summary(self):
        return "Find single-crystal Bragg peaks in MatrixWorkspaces for instruments comprising rectangular detctors (such as SXD at ISIS)."

    def PyInit(self):
        # Input
        self.declareProperty(
            MatrixWorkspaceProperty(
                name="InputWorkspace", defaultValue="", direction=Direction.Input, validator=WorkspaceUnitValidator("TOF")
            ),
            doc="A MatrixWorkspace to integrate (x-axis must be TOF).",
        )
        self.declareProperty(
            IPeaksWorkspaceProperty(name="PeaksWorkspace", defaultValue="", direction=Direction.Output),
            doc="A PeaksWorkspace containing the peaks to integrate.",
        )
        self.declareProperty(
            name="ThresholdIoverSigma",
            defaultValue=5.0,
            direction=Direction.Input,
            validator=FloatBoundedValidator(lower=1.0),
            doc="Threshold value for I/sigma used to identify statistically significant peaks.",
        )
        #   window parameters
        self.declareProperty(
            name="NRows",
            defaultValue=5,
            direction=Direction.Input,
            validator=IntBoundedValidator(lower=2),
            doc="Number of row components in the detector to use in the convolution kernel. "
            "For WISH row components correspond to pixels along a single tube.",
        )
        self.declareProperty(
            name="NCols",
            defaultValue=5,
            direction=Direction.Input,
            validator=IntBoundedValidator(lower=2),
            doc="Number of column components in the detector to use in the convolution kernel. "
            "For WISH column components correspond to tubes.",
        )
        self.declareProperty(
            name="NBins",
            defaultValue=10,
            direction=Direction.Input,
            validator=IntBoundedValidator(lower=2),
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
            "BackToBackExponential peak at the center of each detector panel at the middle of the spectrum.",
        )
        use_nBins = EnabledWhenProperty("GetNBinsFromBackToBackParams", PropertyCriterion.IsDefault)
        self.setPropertySettings("NBins", use_nBins)
        use_nfwhm = EnabledWhenProperty("GetNBinsFromBackToBackParams", PropertyCriterion.IsNotDefault)
        self.setPropertySettings("NFWHM", use_nfwhm)
        # peak validation
        self.declareProperty(
            name="MinFracSize",
            defaultValue=0.125,
            direction=Direction.Input,
            validator=FloatBoundedValidator(lower=0.0),
            doc="Minimum peak size as a fraction of the kernel size.",
        )
        self.declareProperty(
            name="PeakFindingStrategy",
            defaultValue="IOverSigma",
            direction=Direction.Input,
            validator=StringListValidator(["VarianceOverMean", "IOverSigma"]),
            doc="PeakFindingStrategy=IOverSigma will find peaks by integrating data using a shoebox kernel and looking"
            " for peaks with I/sigma > ThresholdIoverSigma. PeakFindingStrategy=VarianceOverMean will look for "
            "peaks with local variance/mean > ThresholdVarianceOverMean.",
        )
        self.declareProperty(
            name="ThresholdVarianceOverMean",
            defaultValue=3.0,
            direction=Direction.Input,
            validator=FloatBoundedValidator(lower=1.0),
            doc="Threshold value for variance/mean used to identify peaks.",
        )
        use_variance_over_mean = EnabledWhenProperty("PeakFindingStrategy", PropertyCriterion.IsNotDefault)
        self.setPropertySettings("ThresholdVarianceOverMean", use_variance_over_mean)
        use_intens_over_sigma = EnabledWhenProperty("PeakFindingStrategy", PropertyCriterion.IsDefault)
        self.setPropertySettings("ThresholdIoverSigma", use_intens_over_sigma)

    def PyExec(self):
        # get input
        ws = self.getProperty("InputWorkspace").value
        nrows = self.getProperty("NRows").value
        ncols = self.getProperty("NCols").value
        nfwhm = self.getProperty("NFWHM").value
        get_nbins_from_b2bexp_params = self.getProperty("GetNBinsFromBackToBackParams").value
        min_frac_size = self.getProperty("MinFracSize").value
        peak_finding_strategy = self.getProperty("PeakFindingStrategy").value

        # create output table workspace
        peaks = self.exec_child_alg("CreatePeaksWorkspace", InstrumentWorkspace=ws, NumberOfPeaks=0, OutputWorkspace="_peaks")

        array_converter = InstrumentArrayConverter(ws)
        banks = ws.getInstrument().findRectDetectors()
        prog_reporter = Progress(self, start=0.0, end=1.0, nreports=len(banks))
        for bank in banks:
            prog_reporter.report(f"Searching in {bank.getName()}")
            # add a dummy peak at center of detector (used in PeakData to get data arrays) - will be deleted after
            irow_to_del = peaks.getNumberPeaks()
            detid = bank[bank.xpixels() // 2][bank.ypixels() // 2].getID()
            ispec = ws.getIndicesFromDetectorIDs([detid])[0]
            xspec = ws.readX(ispec)
            icen = len(xspec) // 2
            self.exec_child_alg("AddPeak", PeaksWorkspace=peaks, RunWorkspace=ws, TOF=xspec[icen], DetectorID=detid)
            dummy_pk = peaks.getPeak(irow_to_del)

            # get nbins of kernel
            if get_nbins_from_b2bexp_params:
                fwhm = get_fwhm_from_back_to_back_params(dummy_pk, ws, detid)
                bin_width = xspec[icen + 1] - xspec[icen]
                nbins = max(3, int(nfwhm * fwhm / bin_width)) if fwhm is not None else self.getProperty("NBins").value
            else:
                nbins = self.getProperty("NBins").value

            # get data in detector coords
            peak_data = array_converter.get_peak_data(dummy_pk, detid, bank.getName(), bank.xpixels(), bank.ypixels(), 1, 1)
            _, y, esq, _ = peak_data.get_data_arrays()  # 3d arrays [rows x cols x tof]
            if peak_finding_strategy == "IOverSigma":
                threshold = self.getProperty("ThresholdIoverSigma").value
                ratio, yconv, econv, kernel_shape = calculate_intens_over_sigma(y, esq, nrows, ncols, nbins)
            else:
                threshold = self.getProperty("ThresholdVarianceOverMean").value
                ratio, ycnts, avg_y, kernel_shape = calculate_variance_over_mean(y, esq, nrows, ncols, nbins)
            # perform final smoothing
            ratio[~np.isfinite(ratio)] = 0
            ratio = uniform_filter1d(ratio, size=3, axis=2, mode="nearest")
            # identify peaks above threshold
            pk_mask = ratio > threshold
            pk_mask = binary_closing(pk_mask)  # removes holes - helps merge close peaks
            labels, nlabels = label(pk_mask)  # identify contiguous nearest-neighbour connected regions
            # identify labels of peaks above min size
            min_size = int(min_frac_size * np.prod(kernel_shape))
            npixels = sum_labels(pk_mask, labels, range(1, nlabels + 1))
            ilabels = np.flatnonzero(npixels > min_size) + 1

            # find index of maximum in I/sigma for each valid peak (label index in ilabels)
            imaxs = maximum_position(ratio, labels, ilabels)

            # add peaks to table
            for ipk in range(len(imaxs)):
                irow, icol, itof = imaxs[ipk]

                # map convolution output to the input
                irow += (kernel_shape[0] - 1) // 2
                icol += (kernel_shape[1] - 1) // 2
                itof += (kernel_shape[2] - 1) // 2

                # find peak position
                # get data in kernel window around index with max I/sigma
                irow_lo = np.clip(irow - kernel_shape[0] // 2, a_min=0, a_max=y.shape[0])
                irow_hi = np.clip(irow + kernel_shape[0] // 2, a_min=0, a_max=y.shape[0])
                icol_lo = np.clip(icol - kernel_shape[1] // 2, a_min=0, a_max=y.shape[1])
                icol_hi = np.clip(icol + kernel_shape[1] // 2, a_min=0, a_max=y.shape[1])
                itof_lo = np.clip(itof - kernel_shape[2] // 2, a_min=0, a_max=y.shape[2])
                itof_hi = np.clip(itof + kernel_shape[2] // 2, a_min=0, a_max=y.shape[2])
                ypk = y[irow_lo:irow_hi, icol_lo:icol_hi, itof_lo:itof_hi]
                if peak_finding_strategy == "VarianceOverMean":
                    # perform additional check as in Winter (2018) https://doi.org/10.1107/S2059798317017235
                    background_cnts = avg_y[tuple(imaxs[ipk])]
                    ypk_cnts = ycnts[irow_lo:irow_hi, icol_lo:icol_hi, itof_lo:itof_hi]
                    if not np.any(ypk_cnts > background_cnts + np.sqrt(background_cnts)):
                        continue  # skip peak
                # integrate over TOF and select detector with max intensity
                imax_det = np.argmax(ypk.sum(axis=2))
                irow_max, icol_max = np.unravel_index(imax_det, ypk.shape[0:-1])
                irow_max += irow_lo  # get index in entire array
                icol_max += icol_lo
                # find max in TOF dimension
                itof_max = np.argmax(ypk.sum(axis=0).sum(axis=0)) + itof_lo

                self.exec_child_alg(
                    "AddPeak",
                    PeaksWorkspace=peaks,
                    RunWorkspace=ws,
                    TOF=xspec[itof_max],
                    DetectorID=int(peak_data.detids[irow_max, icol_max]),
                )
                # set intensity of peak (rough estimate)
                if peak_finding_strategy == "IOverSigma":
                    peak_conv_intens = yconv[tuple(imaxs[ipk])]
                    sig_conv_intens = econv[tuple(imaxs[ipk])]
                    pk = peaks.getPeak(peaks.getNumberPeaks() - 1)
                    bin_width = xspec[itof + 1] - xspec[itof]
                    pk.setIntensity(peak_conv_intens * bin_width)
                    pk.setSigmaIntensity(sig_conv_intens * bin_width)

            # remove dummy peak
            self.exec_child_alg("DeleteTableRows", TableWorkspace=peaks, Rows=[irow_to_del])

            # report number of peaks found
            logger.notice(f"Found {peaks.getNumberPeaks() - irow_to_del} peaks in {bank.getName()}")

        # assign output
        self.setProperty("PeaksWorkspace", peaks)

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


def calculate_intens_over_sigma(y, esq, nrows, ncols, nbins):
    kernel = make_kernel(nrows, ncols, nbins)  # integration shoebox kernel
    yconv = convolve(y, kernel, mode="valid")
    econv = np.sqrt(convolve(esq, kernel**2, mode="valid"))
    with np.errstate(divide="ignore", invalid="ignore"):
        intens_over_sigma = yconv / econv  # ignore 0/0 which produces NaN (recall NaN > x = False)
    return intens_over_sigma, yconv, econv, kernel.shape


def calculate_variance_over_mean(y, esq, nrows, ncols, nbins):
    # Evaluate ratio of Variance/mean (should be 1 for Poisson distribution if constant bg)
    # Peak finding criterion used in DIALS, Winter (2018) https://doi.org/10.1107/S2059798317017235
    with np.errstate(divide="ignore", invalid="ignore"):
        # scale to raw counts
        scale = y / esq
        scale[~np.isfinite(scale)] = 0
        ycnts = y * scale
        # calculate variance = (E[X^2] - E[X]^2)
        avg_y = uniform_filter(ycnts, size=(nrows, ncols, nbins), mode="nearest")
        avg_ysq = uniform_filter(ycnts**2, size=(nrows, ncols, nbins), mode="nearest")
        var_over_mean = (avg_ysq - avg_y**2) / avg_y
    # crop to valid region
    var_over_mean = var_over_mean[
        (nrows - 1) // 2 : -(nrows - 1) // 2, (ncols - 1) // 2 : -(ncols - 1) // 2, (nbins - 1) // 2 : -(nbins - 1) // 2
    ]
    return var_over_mean, ycnts, avg_y, (nrows, ncols, nbins)


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


# register algorithm with mantid
AlgorithmFactory.subscribe(FindSXPeaksConvolve)
