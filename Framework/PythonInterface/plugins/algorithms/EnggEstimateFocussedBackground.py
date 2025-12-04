# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from mantid import AnalysisDataService
from mantid.api import AlgorithmFactory, MatrixWorkspace, MatrixWorkspaceProperty, PythonAlgorithm, Progress
from mantid.kernel import Direction, IntBoundedValidator, FloatBoundedValidator
import numpy as np
from scipy.signal import savgol_filter
from joblib import Parallel, delayed
from functools import lru_cache
from multiprocessing import cpu_count


class EnggEstimateFocussedBackground(PythonAlgorithm):
    MIN_WINDOW_SIZE = 3

    def category(self):
        return "Diffraction\\Engineering"

    def seeAlso(self):
        return ["SmoothData", "DiffractionFocussing"]

    def name(self):
        return "EnggEstimateFocussedBackground"

    def summary(self):
        return "Performs iterative smoothing (low-pass filter) to estimate the background of a spectrum"

    def PyInit(self):
        self.declareProperty(MatrixWorkspaceProperty("InputWorkspace", "", Direction.Input), "Workspace with focussed spectra")

        self.declareProperty(
            MatrixWorkspaceProperty("OutputWorkspace", "", Direction.Output),
            "Workspace to contain the estimated background (one for each spectrum in the InputWorkspace)",
        )

        self.declareProperty(
            name="NIterations",
            defaultValue=50,
            direction=Direction.Input,
            validator=IntBoundedValidator(lower=1),
            doc="Number of iterations of the smoothing procedure to perform. Too few iterations and the background will"
            " be enhanced in the peak regions. Too many iterations and the background will be unrealistically"
            " low and not catch the rising edge at low TOF/d-spacing (typical values are in range 20-100).",
        )

        self.declareProperty(
            name="XWindow",
            defaultValue=600.0,
            direction=Direction.Input,
            validator=FloatBoundedValidator(lower=0.0),
            doc="Extent of the convolution window in the x-axis for all spectra. A reasonable value is about 4-8 times"
            " the FWHM of a typical peak/feature to be suppressed (default is reasonable for TOF spectra). This is "
            "converted to an odd number of points using the median bin width of each spectra.",
        )

        self.declareProperty(
            "ApplyFilterSG",
            True,
            direction=Direction.Input,
            doc="Apply a Savitzky–Golay filter with a linear polynomial over the same XWindow before"
            " the iterative smoothing procedure (recommended for noisy data)",
        )

    def validateInputs(self):
        issues = dict()
        # check there are more than three points in a workspace
        inws = self.getProperty("InputWorkspace").value
        if not isinstance(inws, MatrixWorkspace):
            issues["InputWorkspace"] = "The InputWorkspace must be a MatrixWorkspace."
        elif inws.blocksize() < self.MIN_WINDOW_SIZE:
            issues["InputWorkspace"] = "At least three points needed in each spectra"
        return issues

    def PyExec(self):
        # get input
        inws = self.getProperty("InputWorkspace").value
        niter = self.getProperty("NIterations").value
        xwindow = self.getProperty("XWindow").value
        do_sg_filter = self.getProperty("ApplyFilterSG").value

        # do smoothing in parallel loop over spectra
        nspec = inws.getNumberHistograms()
        prog = Progress(self, start=0.0, end=1.0, nreports=nspec)
        out = Parallel(n_jobs=min(4, cpu_count()), prefer="threads", return_as="generator")(
            delayed(find_bg_of_spectrum)(inws.name(), ispec, xwindow, niter, do_sg_filter, prog) for ispec in range(nspec)
        )

        # make output workspace (with zero errors)
        alg = self.createChildAlgorithm("SetUncertainties", enableLogging=False)
        alg.setProperty("InputWorkspace", inws)
        alg.setProperty("OutputWorkspace", self.getProperty("OutputWorkspace").valueAsStr)
        alg.setProperty("SetError", "zero")
        alg.execute()
        outws = alg.getProperty("OutputWorkspace").value
        # replace intensity in output spectrum with background
        [outws.setY(ispec, yvec) for ispec, yvec in enumerate(out)]

        # set output
        self.setProperty("OutputWorkspace", outws)


def _get_nbins_in_xwindow(x, xwindow):
    binwidth = np.mean(np.diff(x))
    nwindow = max(int(np.ceil(xwindow / binwidth)), 3)
    if not nwindow % 2:
        nwindow += 1
    return nwindow


@lru_cache(maxsize=1)
def _get_end_normalisation(nwindow):
    # This is cached with size 1 because will nwindows will always be the same for ws with common bins
    return np.convolve(np.ones(nwindow), np.ones(nwindow) / nwindow, mode="same")


def find_bg_of_spectrum(wsname, ispec, xwindow, niter, do_sg_filter, prog):
    ws = AnalysisDataService.retrieve(wsname)
    # find kernel size
    nwindow = _get_nbins_in_xwindow(ws.readX(ispec), xwindow)
    # do initial filter to remove very high intensity points
    ybg = ws.readY(ispec)[:]  # copy
    if do_sg_filter:
        ybg = savgol_filter(ybg, nwindow, polyorder=1)
    yavg = ybg.mean()
    ymin = ybg.min()
    threshold = yavg + 2 * (yavg - ymin)
    ybg[ybg > threshold] = threshold
    # perform iterative smoothing
    ybg = do_iterative_smoothing(ybg, nwindow, niter)
    prog.report()
    return ybg


def do_iterative_smoothing(y, nwindow, maxdepth, depth=0):
    """
    Iterative smoothing procedure to estimate the background in powder diffraction as published in
    Bruckner J. Appl. Cryst. (2000). 33, 977-979
    :param y: signal to smooth
    :param n: size of window for convolution
    :param maxdepth: max depth of recursion (i.e. number of iterations)
    :param depth: current iteration
    :return:
    """
    # smooth with hat function
    yy = np.convolve(y, np.ones(nwindow) / nwindow, mode="same")
    # normalise end values effected by convolution
    ends = _get_end_normalisation(nwindow)
    yy[0 : nwindow // 2] = yy[0 : nwindow // 2] / ends[0 : nwindow // 2]
    yy[-nwindow // 2 :] = yy[-nwindow // 2 :] / ends[-nwindow // 2 :]
    if depth < maxdepth:
        # compare pt by pt with original and keep lowest
        idx = yy > y
        yy[idx] = y[idx]
        return do_iterative_smoothing(yy, nwindow, maxdepth, depth + 1)
    else:
        return yy


# register algorithm with mantid
AlgorithmFactory.subscribe(EnggEstimateFocussedBackground)
