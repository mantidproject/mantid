from __future__ import (absolute_import, division, print_function)
from mantid.api import PythonAlgorithm, AlgorithmFactory, MatrixWorkspaceProperty
from mantid.kernel import Direction, IntBoundedValidator
from mantid.simpleapi import Fit
import numpy as np


class FitGaussian(PythonAlgorithm):
    # pylint: disable = no-init

    def category(self):
        return "Optimization"

    def PyInit(self):
        # input
        self.declareProperty(MatrixWorkspaceProperty("Workspace", "", Direction.Input),
                             doc="input workspace")
        self.declareProperty(name="Index",defaultValue=0,validator=IntBoundedValidator(lower=0),
                             doc="workspace index - which spectrum to fit")

        # output
        self.declareProperty(name="PeakCentre",defaultValue=0.,direction=Direction.Output,
                             doc="the centre of the fitted peak")
        self.declareProperty(name="Sigma",defaultValue=0.,direction=Direction.Output,
                             doc="the sigma of the fitted peak; 0. if fitting was not successful")

    def _setOutput(self,peakCentre,sigma):
        self.setProperty("PeakCentre",peakCentre)
        self.setProperty("Sigma",sigma)
        self.log().notice("Fitted Gaussian peak: [" + str(peakCentre) + "," + str(sigma) + "]")

    def _error(self,message):
        self.log().error(message)
        raise RuntimeError(message)

    def _warning(self,message):
        self.log().warning(message)

    def PyExec(self):
        workspace = self.getProperty("Workspace").value
        index     = self.getProperty("Index").value
        nhist     = workspace.getNumberHistograms()

        # index must be in <0,nhist)
        if index >= nhist:
            self._error("Index " + str(index) +
                        " is out of range for the workspace " + workspace.name())

        x_values = np.array(workspace.readX(index))
        y_values = np.array(workspace.readY(index))

        # get peak centre position, assuming that it is the point with the highest value
        imax   = np.argmax(y_values)
        height = y_values[imax]

        # check for zero or negative signal
        if height <= 0.:
            self._warning("Workspace %s, detector %d has maximum <= 0" % (workspace.name(), index))
            return

        # guess sigma (assume the signal is sufficiently smooth)
        # the _only_ peak is at least three samples wide
        # selecting samples above .5 ("full width at half maximum")
        indices  = np.argwhere(y_values > 0.5*height)
        nentries = len(indices)

        if nentries < 3:
            self._warning("Spectrum " + str(index) + " in workspace " + workspace.name() +
                          " has a too narrow peak. Cannot guess sigma. Check your data.")
            return

        minIndex = indices[0,0]
        maxIndex = indices[-1,0]

        # full width at half maximum: fwhm = sigma * (2.*np.sqrt(2.*np.log(2.)))
        fwhm  = np.fabs(x_values[maxIndex] - x_values[minIndex])
        sigma = fwhm / (2.*np.sqrt(2.*np.log(2.)))

        # execute Fit algorithm
        tryCentre = x_values[imax]
        fitFun = "name=Gaussian,PeakCentre=%s,Height=%s,Sigma=%s" % (tryCentre,height,sigma)
        startX = tryCentre - 3.0*fwhm
        endX   = tryCentre + 3.0*fwhm

        # pylint: disable = unpacking-non-sequence, assignment-from-none
        fitStatus, _, _, paramTable = Fit(
            InputWorkspace=workspace, WorkspaceIndex=index,
            Function=fitFun, CreateOutput=True, OutputParametersOnly=True,
            StartX=startX, EndX=endX)

        if not 'success' == fitStatus:
            self._warning("For detector " + str(index) + " in workspace " + workspace.name() +
                          "fit was not successful. Input guess parameters were " + str(fitFun))
            return

        fitParams = paramTable.column(1)
        self._setOutput(fitParams[1],fitParams[2]) # [peakCentre,sigma]

AlgorithmFactory.subscribe(FitGaussian)
