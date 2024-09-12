# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from mantid.api import AlgorithmFactory, MatrixWorkspace, MatrixWorkspaceProperty, PythonAlgorithm, Progress
from mantid.kernel import Direction, IntBoundedValidator, FloatBoundedValidator
import numpy as np
from scipy.signal import savgol_filter


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
            "Workspace to contain the estimated background (one for each spectrum in the " "InputWorkspace)",
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
        doSGfilter = self.getProperty("ApplyFilterSG").value

        # make output workspace
        clone_alg = self.createChildAlgorithm("CloneWorkspace", enableLogging=False)
        clone_alg.setProperty("InputWorkspace", inws)
        clone_alg.setProperty("OutputWorkspace", self.getProperty("OutputWorkspace").valueAsStr)
        clone_alg.execute()
        outws = clone_alg.getProperty("OutputWorkspace").value

        # loop over all spectra
        nbins = inws.blocksize()
        nspec = inws.getNumberHistograms()
        prog_reporter = Progress(self, start=0.0, end=1.0, nreports=nspec)
        for ispec in range(0, nspec):
            prog_reporter.report()

            # get n points in convolution window
            binwidth = np.mean(np.diff(inws.readX(ispec)))
            nwindow = int(np.ceil(xwindow / binwidth))
            if not nwindow % 2:
                nwindow += 1

            if nwindow < self.MIN_WINDOW_SIZE:
                raise RuntimeError("Convolution window must have at least three points")
            elif not nwindow < nbins:
                # not effective due to edge effects of the convolution
                raise RuntimeError("Data has must have at least the number of points as the convolution window")

            # do initial filter to remove very high intensity points
            if doSGfilter:
                ybg = savgol_filter(inws.readY(ispec), nwindow, polyorder=1)
            else:
                ybg = np.copy(inws.readY(ispec))
            Ibar = np.mean(ybg)
            Imin = np.min(ybg)
            ybg[ybg > (Ibar + 2 * (Ibar - Imin))] = Ibar + 2 * (Ibar - Imin)

            # perform iterative smoothing
            ybg = self.doIterativeSmoothing(ybg, nwindow, niter)

            # replace intensity in output spectrum with background
            outws.setY(ispec, ybg)
            outws.setE(ispec, np.zeros(ybg.shape))

        # set output
        self.setProperty("OutputWorkspace", outws)

    def doIterativeSmoothing(self, y, nwindow, maxdepth, depth=0):
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
        yy = np.copy(y)
        yy = np.convolve(yy, np.ones(nwindow) / nwindow, mode="same")
        # normalise end values effected by convolution
        ends = np.convolve(np.ones(nwindow), np.ones(nwindow) / nwindow, mode="same")
        yy[0 : nwindow // 2] = yy[0 : nwindow // 2] / ends[0 : nwindow // 2]
        yy[-nwindow // 2 :] = yy[-nwindow // 2 :] / ends[-nwindow // 2 :]
        if depth < maxdepth:
            # compare pt by pt with original and keep lowest
            idx = yy > y
            yy[idx] = y[idx]
            return self.doIterativeSmoothing(yy, nwindow, maxdepth, depth + 1)
        else:
            return yy


# register algorithm with mantid
AlgorithmFactory.subscribe(EnggEstimateFocussedBackground)
