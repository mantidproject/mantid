# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from mantid import config
from mantid.api import (AlgorithmFactory, MatrixWorkspaceProperty, PythonAlgorithm)
from mantid.simpleapi import (CloneWorkspace)
from mantid.kernel import (Direction, IntBoundedValidator, FloatBoundedValidator, IntListValidator,
                           StringMandatoryValidator)
import numpy as np
from scipy.signal import savgol_filter


class EnggEstimateFocussedBackground(PythonAlgorithm):

    def category(self):
        return "Diffraction\\Engineering"

    def seeAlso(self):
        return ["SmoothData", "DiffractionFocussing"]

    def name(self):
        return "EnggEstimateFocussedBackground"

    def summary(self):
        return "Performs iterative smoothing (low-pass filter) to estimate the background of a spectrum"

    def PyInit(self):
        self.declareProperty(MatrixWorkspaceProperty("InputWorkspace", "", Direction.Input),
                             "Workspace with focussed spectra")

        self.declareProperty(MatrixWorkspaceProperty("OutputWorkspace", "", Direction.Output),
                             "Workspace to contain the estimated background (one for each spectrum in the "
                             "InputWorkspace)")

        self.declareProperty(
            name="XWindow",
            defaultValue=1000,
            direction=Direction.Input,
            validator=FloatBoundedValidator(lower=0.0),
            doc="Extent of the convolution window in the x-axis for all spectra. A reasonable value is about 4-8 times"
                " the FWHM of a typical peak/feature to be suppressed (default is reasonable for TOF spectra). This is "
                "converted to an odd number of points using the median bin width of each spectra.")

        self.declareProperty(
            name="NIterations",
            defaultValue=20,
            direction=Direction.Input,
            validator=IntBoundedValidator(lower=1),
            doc="Number of iterations of the smoothing procedure to perform. Too few iterations and the background will"
                " be enhanced in the peak regions. Too many iterations and the background will be unrealistically"
                " low and not catch the rising edge at low TOF/d-spacing (typical values are in range 10-50).")

        self.declareProperty('ApplyFilterSG', True, direction=Direction.Input,
                             doc='Apply a Savitzky–Golay filter with a linear polynomial over the same XWindow before'
                                 ' the iterative smoothing procedure (recommended for quicker convergence if the data have '
                                 'not been rebunched)')

    def validateInputs(self):
        issues = dict()
        return issues

    def PyExec(self):

        # get input
        inws = self.getProperty("InputWorkspace").value
        xwindow = self.getProperty("XWindow").value
        niter = self.getProperty("NIterations").value
        doSGfilter = self.getProperty('ApplyFilterSG').value

        # make output workspace
        outws = CloneWorkspace(inws)

        # loop over all spectra
        for ispec in range(0, inws.getNumberHistograms()):
            # get n points in convolution window
            binwidth = np.mean(np.diff(inws.readX(ispec)))
            nwindow = int(np.ceil(xwindow / binwidth))
            if not nwindow % 2:
                nwindow += 1

            if doSGfilter:
                ybg = self.doFilter(savgol_filter(inws.readY(ispec), nwindow, polyorder=1), nwindow, niter)
            else:
                ybg = self.doFilter(inws.readY(ispec), nwindow, niter)
            # replace intensity in output spectrum with background
            outws.setY(ispec, ybg)

        # set toutput
        self.setProperty("OutputWorkspace", outws)

    def doFilter(self, y, nwindow, maxdepth, depth=0):
        """
        Iterative smoothing procedure to estimate the background in powder diffraction as published in
        Bruckner J. Appl. Cryst. (2000). 33, 977-979
        :param y: signal to smooth
        :param n: size of window for convolution
        :param maxdepth: max depth of recursion (i.e. number of iterations)
        :param depth: current iteration
        :return:
        """
        if depth < maxdepth:
            # step 1 - replace v. high intensity points
            Ibar = np.mean(y)
            Imin = np.min(y)
            yy = np.copy(y)
            yy[y > (Ibar + 2 * (Ibar - Imin))] = (Ibar + 2 * (Ibar - Imin))
            # 2 - smooth with hat function
            yy = np.convolve(yy, np.ones(nwindow) / nwindow, mode="same")
            # 3 - compare pt by pt with original and keep lowest
            idx = yy > y
            yy[idx] = y[idx]
            return self.doFilter(yy, nwindow, maxdepth, depth + 1)
        else:
            # perform final smoothing
            return np.convolve(y, np.ones(nwindow) / nwindow, mode="same")
