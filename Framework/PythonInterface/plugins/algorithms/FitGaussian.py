from mantid.api import PythonAlgorithm, AlgorithmFactory, AlgorithmManager, MatrixWorkspaceProperty
from mantid.kernel import Direction, IntBoundedValidator

class FitGaussian(PythonAlgorithm):

    def category(self):
        return "Optimization"

    def PyInit(self):
        # input
        self.declareProperty(
            MatrixWorkspaceProperty("Workspace", "", Direction.Input),
            doc="input workspace")
        self.declareProperty(name="Index",
            defaultValue=0, validator=IntBoundedValidator(lower=0),
            doc="workspace index - which spectrum to fit")

        # output
        self.declareProperty(name="PeakCentre",
            defaultValue=0.,
            direction = Direction.Output,
            doc="the centre of the fitted peak")
        self.declareProperty(name="Sigma",
            defaultValue=0.,
            direction = Direction.Output,
            doc="the sigma of the fitted peak; 0. if fitting was not successful")

    def _setOutput(self,centre,sigma):
        self.setProperty("PeakCentre", centre)
        self.setProperty("Sigma",      sigma)
        self.log().notice("Gaussian fitted peak: [" + str(centre) + "," + str(sigma) + "]")

    def _error(self,message):
        self.log().error(message)
        raise RuntimeError(message)

    def _warning(self,message):
        self.log().warning(message)

    def PyExec(self):
        from mantid.simpleapi import Fit
        import numpy as np

        workspace = self.getProperty("Workspace").value
        index     = self.getProperty("Index").value
        nhist     = workspace.getNumberHistograms()

        if index >= nhist:
            self._error("Index " + str(index) +
                        " is out of range for the workspace " + workspace.getName())

        x_values = np.array(workspace.readX(index))
        y_values = np.array(workspace.readY(index))

        # get peak centre position; assuming that it is the point with the highest value
        imax   = np.argmax(y_values)
        height = y_values[imax]

        # check for zero or negative signal
        if height <= 0.:
            self._warning("Workspace %s, detector %d has maximum <= 0" % (workspace.getName(), index))
            return

        try_centre = x_values[imax]

        # guess sigma
        indices  = np.argwhere(y_values > 0.5*height)
        nentries = len(indices)

        if nentries < 3:
            self._error("Spectrum " + str(index) + " in workspace " + workspace.getName() +
                        " has too narrow peak. Cannot guess sigma. Check your data.")

        # fwhm = sigma * (2.*np.sqrt(2.*np.log(2.)))
        fwhm  = np.fabs(x_values[indices[nentries - 1, 0]] - x_values[indices[0, 0]])
        sigma = fwhm/(2.*np.sqrt(2.*np.log(2.)))

        # create and execute Fit algorithm
        myfunc = ("name=Gaussian, Height=" + str(height) +
                  ", PeakCentre=" + str(try_centre) + ", Sigma=" + str(sigma))
        startX = try_centre - 3.0*fwhm
        endX   = try_centre + 3.0*fwhm

        fit_result = Fit(Function=myfunc, InputWorkspace=workspace,
                         CreateOutput=True, OutputParametersOnly=True,
                         StartX=startX, EndX=endX)

        fit_success = ('success' == fit_result[0])

        if not fit_success:
            self._warning("For detector " + str(index) + " in workspace " + workspace.getName() +
                          "fit was not successful. Input guess parameters are " + str(myfunc))
            return

        fit_params = fit_result[3].column(1)
        self._setOutput(fit_params[1],fit_params[2]) # [peak centre,sigma]

AlgorithmFactory.subscribe(FitGaussian)
